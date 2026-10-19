#!/usr/bin/env python3
# Gerbera - https://gerbera.io/
# Copyright (C) 2026 Gerbera Contributors
# SPDX-License-Identifier: GPL-2.0-only
"""Compare WPL/XSPF fixtures with real Gerbera ContentDirectory results."""

import argparse
import contextlib
import hashlib
import io
import json
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import tempfile
import time
import urllib.parse
import urllib.request
import wave
import xml.etree.ElementTree as ET

DC = "{http://purl.org/dc/elements/1.1/}"
UP = "{urn:schemas-upnp-org:metadata-1-0/upnp/}"
SERVICE = "urn:schemas-upnp-org:service:ContentDirectory:1"
HTTP = urllib.request.build_opener(urllib.request.ProxyHandler({}))


def wav_bytes(title):
    stream = io.BytesIO()
    with wave.open(stream, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(8000)
        wav.writeframes(b"\0\0" * 800)
    data = stream.getvalue()
    text = title.encode() + b"\0"
    info = b"INFOINAM" + struct.pack("<I", len(text)) + text + b"\0" * (len(text) % 2)
    data += b"LIST" + struct.pack("<I", len(info)) + info
    return data[:4] + struct.pack("<I", len(data) - 8) + data[8:]


class Fixture:
    def __init__(self, args, root, mode):
        self.args, self.root, self.mode = args, root, mode
        self.src = root / "src"
        self.pl = self.src / "pl"
        self.pl.mkdir(parents=True)
        self.corpus = args.source_root / "test/scripting/fixtures/xml-playlists"
        self.manifest = json.loads((self.corpus / "cases.json").read_text())
        self.files = {}
        self.titles = {}
        for key, relative in self.manifest["media"].items():
            path = self.src / relative
            path.parent.mkdir(exist_ok=True)
            if key == "V":
                shutil.copyfile(self.corpus / "video.mp4", path)
                self.titles[key] = "v"
            else:
                self.titles[key] = "Äther 東京" if key == "U" else "Track " + key
                path.write_bytes(wav_bytes(self.titles[key]))
            self.files[key] = path
        self.expected = {}
        for case in self.manifest["cases"]:
            name = Path(case["file"])
            data = (self.corpus / name).read_bytes()
            encoding = "utf-16" if name.stem == "w04" else "utf-8"
            text = data.decode(encoding).replace("/fixture/xml", str(self.src))
            (self.pl / name).write_bytes(text.encode(encoding))
            self.expected[name.stem] = (
                case["expected"] if case["outcome"] == "import" else []
            )
        order = ["A", "V", "B", "V", "A"]
        paths = ["../" + self.manifest["media"][key] for key in order]
        for ext in ["m3u", "m3u8", "pls", "asx"]:
            name = "legacy" + ext
            if ext == "asx":
                doc = ET.Element("asx", version="3.0")
                for path in paths:
                    ET.SubElement(ET.SubElement(doc, "entry"), "ref", href=path)
                text = ET.tostring(doc, encoding="unicode")
            elif ext == "pls":
                text = "[playlist]\n" + "".join(
                    f"File{i}={p}\n" for i, p in enumerate(paths, 1)
                )
            else:
                text = "\n".join(paths)
            (self.pl / (name + "." + ext)).write_text(text, encoding="utf-8")
            self.expected[name] = order
        self.ip = args.ip or socket.gethostbyname(socket.gethostname())
        for port in range(49270, 49370):
            with socket.socket() as probe:
                try:
                    probe.bind((self.ip, port))
                except OSError:
                    continue
                self.port = port
                self.host = f"http://{self.ip}:{port}"
                break
        else:
            raise RuntimeError("No free fixture port")
        config_text = subprocess.check_output(
            [str(args.binary), "--create-config"]
        ).decode()
        config = ET.fromstring(config_text[config_text.index("<?xml") :])
        ns = config.tag.split("}")[0] + "}"
        ET.register_namespace("", ns[1:-1])

        def set_value(path, value):
            node = config
            for part in path.split("/"):
                tag = ns + part
                child = node.find(tag)
                if child is None:
                    child = ET.SubElement(node, tag)
                node = child
            node.text = str(value)
            return node

        web = root / "web"
        web.mkdir()
        set_value("server/home", root)
        set_value("server/webroot", web)
        set_value("server/storage/sqlite3/database-file", root / "db.sqlite")
        for tag, name in [
            ("init-sql-file", "sqlite3.sql"),
            ("upgrade-file", "sqlite3-upgrade.xml"),
            ("drop-file", "sqlite3-drop.sql"),
        ]:
            set_value(
                "server/storage/sqlite3/" + tag,
                args.source_root / "src/database/sqlite3" / name,
            )
        set_value("server/storage/sqlite3/backup", "").set("enabled", "no")
        set_value("server/ui", "").set("enabled", "yes")
        set_value("server/ui/accounts", "").set("enabled", "no")
        set_value("import/filesystem-charset", "UTF-8")
        # XML must ignore this legacy text-playlist charset.
        set_value("import/playlist-charset", "windows-1252")
        set_value(
            "import/scripting/script-folder/common", args.source_root / "scripts/js"
        )
        autoscan = set_value("import/autoscan", "")
        autoscan.set("use-inotify", "no")
        ET.SubElement(
            autoscan,
            ns + "directory",
            {
                "location": str(self.src),
                "mode": "timed",
                "interval": "3600",
                "recursive": "yes",
                "import-mode": mode,
            },
        )
        mappings = set_value("import/mappings/extension-mimetype", "")
        mappings.set("ignore-unknown", "no")
        # ASX uses the existing explicit MIME override; WPL/XSPF use new defaults.
        ET.SubElement(mappings, ns + "map", {"from": "asx", "to": "video/x-ms-asx"})
        self.config = root / "config.xml"
        ET.ElementTree(config).write(
            self.config, encoding="utf-8", xml_declaration=True
        )
        self.process = None
        self.checks = []

    def api(self, **params):
        with HTTP.open(
            self.host + "/interface?" + urllib.parse.urlencode(params), timeout=5
        ) as response:
            result = json.load(response)
        assert result.get("success"), result
        return result

    def browse(self, oid, flag="BrowseDirectChildren"):
        envelope = ET.Element("{http://schemas.xmlsoap.org/soap/envelope/}Envelope")
        body = ET.SubElement(
            envelope, "{http://schemas.xmlsoap.org/soap/envelope/}Body"
        )
        call = ET.SubElement(body, "{" + SERVICE + "}Browse")
        for key, value in dict(
            ObjectID=oid,
            BrowseFlag=flag,
            Filter="*",
            StartingIndex=0,
            RequestedCount=1000,
            SortCriteria="",
        ).items():
            ET.SubElement(call, key).text = str(value)
        request = urllib.request.Request(
            self.host + "/upnp/control/cds",
            data=ET.tostring(envelope),
            headers={
                "Content-Type": "text/xml; charset=utf-8",
                "SOAPAction": '"' + SERVICE + '#Browse"',
            },
        )
        with HTTP.open(request, timeout=5) as response:
            result = ET.fromstring(response.read()).findtext(".//Result")
        return list(ET.fromstring(result))

    def wait(self, check):
        end, last = time.monotonic() + 30, None
        while time.monotonic() < end:
            if self.process.poll() is not None:
                raise AssertionError("Gerbera exited before fixture completed")
            try:
                if check():
                    return
            except (OSError, ValueError, AssertionError, ET.ParseError) as error:
                last = error
            time.sleep(0.15)
        raise AssertionError(f"Fixture timeout: {last}")

    def assert_views(self):
        roots = [n for n in self.browse(0) if n.findtext(DC + "title") == "Playlists"]
        assert len(roots) == 1
        pending = [roots[0].get("id")]
        found = {}
        while pending:
            for node in self.browse(pending.pop()):
                if node.findtext(UP + "class") == "object.container.playlistContainer":
                    found.setdefault(node.findtext(DC + "title"), []).append(node)
                elif node.tag.endswith("}container"):
                    pending.append(node.get("id"))
        assert not (set(found) - set(self.expected)), set(found) - set(self.expected)
        for name, expected in self.expected.items():
            views = found.get(name, [])
            assert len(views) == 2 or (not expected and len(views) == 0), (
                name,
                len(views),
            )
            for view in views:
                items = self.browse(view.get("id"))
                assert len(items) == len(expected), (name, len(items), expected)
                for item, key in zip(items, expected):
                    resource = next(n for n in item if n.tag.endswith("}res"))
                    if key in ["HTTPS", "RELATIVE_HTTPS"]:
                        remote = {
                            "w09": "https://media.example.invalid/a.mp3?x=1&y=2",
                            "x17": "https://media.example.invalid/a.mp3?token=one%26two&x=1",
                            "x19": "https://media.example.invalid/media/a.wav",
                            "x23": "https://media.example.invalid/a.mp3?title=Why%3F&token=a+b",
                        }
                        assert resource.text == remote[name], resource.text
                        if name == "x23":
                            assert item.findtext(DC + "title") == "Why? Äther 東京"
                        continue  # Never contact external fixture URLs.
                    assert item.findtext(DC + "title") == self.titles[key], (
                        name,
                        key,
                        item.findtext(DC + "title"),
                    )
                    cls = item.findtext(UP + "class")
                    assert cls.startswith(
                        "object.item.videoItem"
                        if key == "V"
                        else "object.item.audioItem"
                    ), cls
                    with HTTP.open(resource.text, timeout=5) as response:
                        data = response.read()
                    assert (
                        hashlib.sha256(data).digest()
                        == hashlib.sha256(self.files[key].read_bytes()).digest()
                    ), (name, key)
        return True

    def rescan(self):
        sid = self.api(req_type="auth", action="get_sid")["GerberaSID"]
        scan = next(
            x
            for x in self.api(req_type="autoscan", action="list", GerberaSID=sid)[
                "autoscans"
            ]
            if x["location"] == str(self.src)
        )
        self.api(
            req_type="autoscan",
            action="as_run",
            object_id=scan["objectID"],
            from_fs="false",
            GerberaSID=sid,
        )
        time.sleep(0.5)

    def record(self, name):
        self.wait(self.assert_views)
        self.checks.append(name)
        print(self.mode, name, "PASS", flush=True)

    def start(self):
        self.log = (self.root / ("server-" + str(len(self.checks)) + ".log")).open("wb")
        self.process = subprocess.Popen(
            [
                str(self.args.binary),
                "-c",
                str(self.config),
                "-i",
                self.ip,
                "-p",
                str(self.port),
            ],
            stdout=self.log,
            stderr=subprocess.STDOUT,
        )
        self.wait(lambda: self.browse(0))

    def stop(self):
        if self.process is not None:
            self.process.terminate()
            try:
                self.process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            self.log.close()
            self.process = None

    def run(self):
        try:
            self.start()
            self.record(
                f"{len(self.manifest['cases'])} XML cases and four legacy formats, both views and full resources"
            )
            self.rescan()
            self.record("unchanged rescan preserves all sequences")
            self.rescan()
            self.record("second rescan is idempotent")
            self.stop()
            self.start()
            self.record("restart retains all playlist contents")
            return self.checks
        except BaseException:
            self.log.flush()
            print(Path(self.log.name).read_text(errors="replace")[-7000:], flush=True)
            raise
        finally:
            self.stop()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--ip")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--work-directory", type=Path)
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.source_root = args.source_root.resolve()
    if args.work_directory:
        args.work_directory = args.work_directory.resolve()
    work = (
        contextlib.nullcontext(str(args.work_directory))
        if args.work_directory
        else tempfile.TemporaryDirectory(prefix="grb-x-")
    )
    results = {}
    with work as directory:
        for mode in ["mt", "grb"]:
            root = Path(directory) / mode
            root.mkdir(parents=True)
            results[mode] = Fixture(args, root, mode).run()
    if args.output:
        args.output.write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
