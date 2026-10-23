#!/usr/bin/env python3
# Gerbera - https://gerbera.io/
# Copyright (C) 2026 Gerbera Contributors
#
# Gerbera is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# Gerbera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Gerbera. If not, see <http://www.gnu.org/licenses/>.

"""Exercise real file changes and ContentDirectory results on an isolated Gerbera."""

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import time
import urllib.parse
import urllib.request
import wave
import xml.etree.ElementTree as ET

from integration_runtime import HTTP, GerberaProcess, fixture_workspace

DC = "{http://purl.org/dc/elements/1.1/}"
UP = "{urn:schemas-upnp-org:metadata-1-0/upnp/}"
SERVICE = "urn:schemas-upnp-org:service:ContentDirectory:1"


def wav_bytes(title, frames=800):
    data = io.BytesIO()
    with wave.open(data, "wb") as stream:
        stream.setnchannels(1)
        stream.setsampwidth(2)
        stream.setframerate(8000)
        stream.writeframes(b"\0\0" * frames)
    result = data.getvalue()
    text = title.encode() + b"\0"
    chunk = b"INAM" + struct.pack("<I", len(text)) + text + b"\0" * (len(text) % 2)
    info = b"INFO" + chunk
    result += b"LIST" + struct.pack("<I", len(info)) + info
    return result[:4] + struct.pack("<I", len(result) - 8) + result[8:]


class Fixture:
    def __init__(
        self, args, root, import_mode, scan_mode="inotify", legacy_wrapper=False
    ):
        self.root = root
        self.src = root / "src"
        self.audio = self.src / "audio"
        self.playlists = self.src / "pl"
        self.audio.mkdir(parents=True)
        self.playlists.mkdir()
        self.files = {"A": self.audio / "a.wav", "B": self.audio / "b.wav"}
        self.titles = {"A": "Track A", "B": "Track B"}
        self.frames = {"A": 800, "B": 800}
        for key, path in self.files.items():
            path.write_bytes(wav_bytes(self.titles[key]))
        self.order = ["A", "B", "A"]
        self.names = {"one": "m3u", "two": "m3u8", "three": "pls", "four": "asx"}
        self.empty = set()
        self.write_playlists()
        self.binary = args.binary
        self.debug = args.debug
        self.scan_mode = scan_mode
        self.startup_timeout = args.startup_timeout
        self.ip = args.ip or socket.gethostbyname(socket.gethostname())
        xml = subprocess.check_output([str(self.binary), "--create-config"]).decode()
        config = ET.fromstring(xml[xml.index("<?xml") :])
        ns = config.tag.split("}")[0][1:]
        ET.register_namespace("", ns)

        def set_value(path, value):
            node = config
            for part in path.split("/"):
                tag = "{" + ns + "}" + part
                child = node.find(tag)
                if child is None:
                    child = ET.SubElement(node, tag)
                node = child
            node.text = str(value)
            return node

        set_value("server/home", root)
        web = root / "web"
        web.mkdir()
        set_value("server/webroot", web)
        set_value("server/storage/sqlite3/database-file", root / "db.sqlite")
        set_value(
            "server/storage/sqlite3/init-sql-file",
            args.source_root / "src/database/sqlite3/sqlite3.sql",
        )
        set_value(
            "server/storage/sqlite3/upgrade-file",
            args.source_root / "src/database/sqlite3/sqlite3-upgrade.xml",
        )
        set_value(
            "server/storage/sqlite3/drop-file",
            args.source_root / "src/database/sqlite3/sqlite3-drop.sql",
        )
        set_value("server/storage/sqlite3/backup", "").set("enabled", "no")
        set_value("server/ui", "").set("enabled", "yes")
        set_value("server/ui/accounts", "").set("enabled", "no")
        set_value("import/filesystem-charset", "UTF-8")
        set_value("import/playlist-charset", "UTF-8")
        set_value(
            "import/scripting/script-folder/common", args.source_root / "scripts/js"
        )
        if legacy_wrapper:
            custom = root / "js"
            custom.mkdir()
            (custom / "legacy.js").write_text(
                "function legacyPlaylist(obj, cont, root, id, type) { importPlaylist(obj, cont, root, id, type); }",
                encoding="utf-8",
            )
            set_value("import/scripting/script-folder/custom", custom)
            set_value("import/scripting/import-function/playlist", "legacyPlaylist")
        autoscan = set_value("import/autoscan", "")
        autoscan.set("use-inotify", "yes" if scan_mode == "inotify" else "no")
        autoscan.set("inotify-attrib", "no")
        ET.SubElement(
            autoscan,
            "{" + ns + "}directory",
            {
                "location": str(self.src),
                "mode": scan_mode,
                "interval": "3600",
                "recursive": "yes",
                "import-mode": import_mode,
                "hidden-files": "no",
            },
        )
        extension = set_value("import/mappings/extension-mimetype", "")
        extension.set("ignore-unknown", "no")
        ET.SubElement(
            extension, "{" + ns + "}map", {"from": "asx", "to": "video/x-ms-asx"}
        )
        self.config = root / "config.xml"
        ET.ElementTree(config).write(
            self.config, encoding="utf-8", xml_declaration=True
        )
        self.server = GerberaProcess(
            self.binary, self.config, self.ip, self.root, debug=self.debug
        )
        self.playlist_root = None
        self.checks = []

    def write_playlists(self):
        for name, ext in self.names.items():
            paths = (
                ["../audio/" + self.files[key].name for key in self.order]
                if name not in self.empty
                else []
            )
            if ext in ("m3u", "m3u8"):
                data = "\n".join(paths) + "\n"
            elif ext == "pls":
                data = "[playlist]\n" + "".join(
                    f"File{i}={p}\n" for i, p in enumerate(paths, 1)
                )
                data += f"NumberOfEntries={len(paths)}\nVersion=2\n"
            else:
                tree = ET.Element("asx", version="3.0")
                for path in paths:
                    ET.SubElement(ET.SubElement(tree, "entry"), "ref", href=path)
                data = ET.tostring(tree, encoding="unicode")
            (self.playlists / (name + "." + ext)).write_text(data, encoding="utf-8")

    def api(self, **params):
        url = self.host + "/interface?" + urllib.parse.urlencode(params)
        with HTTP.open(url, timeout=5) as response:
            result = json.load(response)
        assert result.get("success"), result
        return result

    @property
    def host(self):
        return self.server.host

    def soap_response(self, action, params):
        envelope = ET.Element("{http://schemas.xmlsoap.org/soap/envelope/}Envelope")
        body = ET.SubElement(
            envelope, "{http://schemas.xmlsoap.org/soap/envelope/}Body"
        )
        call = ET.SubElement(body, "{" + SERVICE + "}" + action)
        for key, value in params.items():
            ET.SubElement(call, key).text = str(value)
        request = urllib.request.Request(
            self.host + "/upnp/control/cds",
            data=ET.tostring(envelope),
            headers={
                "Content-Type": "text/xml; charset=utf-8",
                "SOAPAction": '"' + SERVICE + "#" + action + '"',
            },
        )
        with HTTP.open(request, timeout=5) as response:
            return ET.fromstring(response.read())

    def soap(self, action, params):
        result = self.soap_response(action, params).findtext(".//Result")
        return list(ET.fromstring(result))

    def search(self, container, criteria):
        return self.soap(
            "Search",
            dict(
                ContainerID=container,
                SearchCriteria=criteria,
                Filter="*",
                StartingIndex=0,
                RequestedCount=1000,
                SortCriteria="",
            ),
        )

    def browse(self, oid):
        return self.soap(
            "Browse",
            dict(
                ObjectID=oid,
                BrowseFlag="BrowseDirectChildren",
                Filter="*",
                StartingIndex=0,
                RequestedCount=1000,
                SortCriteria="",
            ),
        )

    def sources(self):
        items = self.search(2, 'upnp:class derivedfrom "object.item.audioItem"')
        return {x.findtext(DC + "title"): x.get("id") for x in items}

    def assert_views(self):
        if self.playlist_root is None:
            found = self.search(
                0, 'upnp:class derivedfrom "object.container.playlistContainer"'
            )
            if not found:
                return False
            parent = self.soap(
                "Browse",
                dict(
                    ObjectID=found[0].get("parentID"),
                    BrowseFlag="BrowseMetadata",
                    Filter="*",
                    StartingIndex=0,
                    RequestedCount=1,
                    SortCriteria="",
                ),
            )[0]
            self.playlist_root = parent.get("parentID")
        pending = [self.playlist_root]
        views = []
        while pending:
            for node in self.browse(pending.pop()):
                if node.findtext(UP + "class") == "object.container.playlistContainer":
                    views.append(node)
                elif node.tag.endswith("}container"):
                    pending.append(node.get("id"))
        actual_names = [x.findtext(DC + "title") for x in views]
        assert set(actual_names) <= set(self.names), actual_names
        for name in self.names:
            assert actual_names.count(name) == 2 or (
                name in self.empty and actual_names.count(name) == 0
            ), actual_names
        for view in views:
            items = self.browse(view.get("id"))
            order = self.order if view.findtext(DC + "title") not in self.empty else []
            assert [x.findtext(DC + "title") for x in items] == [
                self.titles[k] for k in order
            ], [(x.get("id"), x.findtext(DC + "title")) for x in items]
            for item, key in zip(items, order):
                element = next(x for x in item if x.tag.endswith("}res"))
                assert int(element.get("size")) == self.files[key].stat().st_size
                duration = element.get("duration").split(":")
                seconds = (
                    int(duration[0]) * 3600 + int(duration[1]) * 60 + float(duration[2])
                )
                assert abs(seconds - self.frames[key] / 8000) < 0.002, element.attrib
                resource = element.text
                with HTTP.open(resource, timeout=5) as response:
                    payload = response.read()
                assert (
                    hashlib.sha256(payload).digest()
                    == hashlib.sha256(self.files[key].read_bytes()).digest()
                )
        return True

    def record(self, name, check=None):
        self.server.wait(check or self.assert_views)
        self.checks.append(name)
        print(name, "PASS", flush=True)

    def start(self):
        def ready():
            ping = self.soap_response("GetSystemUpdateID", {}).findtext(".//Id")
            assert ping is not None and ping.isdecimal(), ping
            self.browse(0)  # A valid empty root is ready, too.

        self.server.start(ready, timeout=self.startup_timeout)
        if self.scan_mode == "inotify":
            self.server.wait_for_watches((self.src, self.audio, self.playlists))

    def stop(self):
        self.server.stop()

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

    def run_compat(self):
        try:
            self.start()
            self.record("legacy wrapper without returned IDs initially imports")
            ids = self.sources()
            with self.files["B"].open("r+b") as stream:
                stream.read()
            time.sleep(0.5)
            self.record(
                "legacy wrapper without returned IDs does not prune existing positions"
            )
            assert self.sources() == ids
            return self.checks
        finally:
            self.stop()

    def run_timed(self):
        try:
            self.start()
            self.record("initial timed import")
            ids = self.sources()
            time.sleep(1.1)
            self.titles["B"] = "Scanned B"
            self.files["B"].write_bytes(wav_bytes(self.titles["B"], self.frames["B"]))
            self.rescan()
            self.record(
                "rescan updates music metadata and dependent playlists without inotify"
            )
            assert self.sources()[self.titles["B"]] == ids["Track B"]
            assert self.sources()["Track A"] == ids["Track A"]
            self.order = ["B", "A", "B", "A"]
            self.write_playlists()
            self.rescan()
            self.record(
                "rescan imports playlist reorder and repetitions without inotify"
            )
            self.rescan()
            self.record("repeated timed-directory rescan is idempotent")
            self.stop()
            self.start()
            self.record("restart retains timed-directory playlist contents")
            assert self.sources()[self.titles["B"]] == ids["Track B"]
            assert self.sources()["Track A"] == ids["Track A"]
            self.files["B"].unlink()
            self.order = ["A", "A"]
            self.rescan()
            self.record(
                "rescan removes deleted music and its playlist positions without inotify"
            )
            return self.checks
        finally:
            self.stop()

    def run(self):
        try:
            self.start()
            self.record(
                "initial import: four formats, repeated tracks, both views and full resources"
            )
            ids = self.sources()
            before = self.files["B"].read_bytes(), self.files["B"].stat().st_mtime_ns
            with self.files["B"].open("rb") as stream:
                stream.read()
            time.sleep(0.5)
            self.record("read-only close")
            with self.files["B"].open("r+b") as stream:
                stream.read()
            time.sleep(0.5)
            self.record("writable close without writes")
            assert self.sources() == ids
            assert before == (
                self.files["B"].read_bytes(),
                self.files["B"].stat().st_mtime_ns,
            )
            self.titles["B"] = "Changed B"
            self.files["B"].write_bytes(wav_bytes(self.titles["B"], self.frames["B"]))
            self.record("in-place music title change")
            assert self.sources()[self.titles["B"]] == ids["Track B"]
            self.order = ["B", "A", "B", "A", "A"]
            self.write_playlists()
            self.record("playlist reorder, addition and multiple repetitions")
            self.order = ["A", "B"]
            self.write_playlists()
            self.record("playlist entry removal")
            old_mtime = self.files["B"].stat().st_mtime_ns
            self.titles["B"] = "Atomic B"
            replacement = self.root / ".replacement"
            self.frames["B"] = 1600
            replacement.write_bytes(wav_bytes(self.titles["B"], self.frames["B"]))
            os.utime(replacement, ns=(old_mtime, old_mtime))
            replacement.replace(self.files["B"])
            self.record("atomic music replacement with preserved mtime")
            assert self.sources()[self.titles["B"]] == ids["Track B"]
            for i in range(2):
                self.rescan()
                self.record(f"normal rescan {i + 1}")
                assert self.sources()[self.titles["B"]] == ids["Track B"]
            self.stop()
            self.titles["B"] = "Offline B"
            time.sleep(1.1)
            self.files["B"].write_bytes(wav_bytes(self.titles["B"], self.frames["B"]))
            self.start()
            self.record("restart after offline music change")
            assert self.sources()[self.titles["B"]] == ids["Track B"]
            self.files["C"] = self.audio / "c.wav"
            self.titles["C"] = "Track C"
            self.frames["C"] = 800
            self.files["C"].write_bytes(wav_bytes(self.titles["C"]))
            self.order = ["C", "B", "A", "C"]
            self.write_playlists()
            self.record("new music file and playlist references to it")
            self.names["five"] = "m3u8"
            self.write_playlists()
            self.record("new playlist file")
            self.names["renamed"] = self.names.pop("one")
            (self.playlists / "one.m3u").rename(self.playlists / "renamed.m3u")
            self.record("playlist file rename removes old view")
            self.empty.add("renamed")
            self.write_playlists()
            self.record("empty playlist clears former contents")
            self.empty.clear()
            self.write_playlists()
            self.record("refill empty playlist")
            self.names.pop("three")
            (self.playlists / "three.pls").unlink()
            self.record("playlist file deletion")
            self.files["B"].unlink()
            self.order = [key for key in self.order if key != "B"]
            self.record("real music deletion removes unavailable entries")
            self.rescan()
            self.record("rescan does not resurrect deleted files")
            return self.checks
        finally:
            self.stop()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--ip")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--work-directory", type=Path)
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--startup-timeout", type=float, default=30)
    args = parser.parse_args()
    results = {}
    with fixture_workspace(args.work_directory, prefix="grb-pl-") as work:
        for mode in ("mt", "grb"):
            root = Path(work) / mode
            root.mkdir()
            results[mode + "-inotify"] = Fixture(args, root, mode).run()
            timed = Path(work) / (mode + "-timed")
            timed.mkdir()
            results[mode + "-timed"] = Fixture(args, timed, mode, "timed").run_timed()
            legacy = Path(work) / (mode + "-legacy")
            legacy.mkdir()
            results[mode + "-legacy"] = Fixture(
                args, legacy, mode, legacy_wrapper=True
            ).run_compat()
    if args.output:
        args.output.write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
