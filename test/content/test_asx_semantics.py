#!/usr/bin/env python3
# Gerbera - https://gerbera.io/
# Copyright (C) 2026 Gerbera Contributors
# SPDX-License-Identifier: GPL-2.0-only
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
"""Exercise ASX structure semantics through a real isolated ContentDirectory."""

import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import shutil
import threading
import time
import xml.etree.ElementTree as ET

from integration_runtime import fixture_workspace
from test_xml_playlists import DC, UP, HTTP, Fixture, wav_bytes


class AsxSemanticsFixture(Fixture):
    def __init__(self, args, root, mode, charset):
        super().__init__(args, root, mode)
        shutil.rmtree(self.src)
        self.src.mkdir()
        corpus = args.source_root / "test/scripting/fixtures/asx-semantics"
        self.manifest = json.loads((corpus / "cases.json").read_text())
        self.expected = {}
        self.pending_playlists = {}
        self.case_files = {}
        self.title_overrides = {}
        self.deleted = set()
        self.checks = []
        self.view_checks = 0
        self.resource_checks = 0
        self.remote_requests = []
        requests = self.remote_requests

        class RemoteTrap(BaseHTTPRequestHandler):
            def do_GET(self):
                requests.append(self.path)
                self.send_error(500)

            def log_message(self, *args):
                pass

        self.remote = ThreadingHTTPServer((self.ip, 0), RemoteTrap)
        self.remote_thread = threading.Thread(
            target=self.remote.serve_forever, daemon=True
        )
        self.remote_thread.start()
        remote_host = f"http://{self.ip}:{self.remote.server_port}"
        self.remote_media = remote_host + "/a.mp3?title=Why%3F&token=a+b"

        def hidden_children(text):
            return text.replace('"sub/', '".sub/').replace('"depth/', '".depth/')

        for case in self.manifest["cases"]:
            name = case["id"]
            directory = self.src / name
            directory.mkdir()
            files = {}
            for key, media in self.manifest["media"].items():
                if "path" not in media:
                    continue
                path = directory / media["path"]
                path.parent.mkdir(exist_ok=True)
                if key == "V":
                    shutil.copyfile(self.corpus / "video.mp4", path)
                else:
                    path.write_bytes(wav_bytes(media["title"]))
                files[key] = path
            data = (corpus / case["file"]).read_bytes()
            assert hashlib.sha256(data).hexdigest() == case["sha256"]
            encoding = case["encoding"]
            text = hidden_children(data.decode(encoding))
            text = text.replace(
                self.manifest["media"]["HTTPS"]["uri"].replace("&", "&amp;"),
                self.remote_media.replace("&", "&amp;"),
            )
            text = text.replace(
                "https://playlist.example.invalid/list.asx", remote_host + "/child.asx"
            )
            text = text.replace(
                "https://presentation.example.invalid/image", remote_host + "/image"
            )
            text = text.replace("https://log.example.invalid/", remote_host + "/log")
            self.pending_playlists[directory / case["file"]] = text.encode(encoding)
            for child, digest in self.manifest["dependencies"].items():
                contents = (corpus / child).read_bytes()
                assert hashlib.sha256(contents).hexdigest() == digest
                path = directory / ("." + child)
                path.parent.mkdir(exist_ok=True)
                path.write_text(hidden_children(contents.decode()), encoding="utf-8")
            self.expected[name] = case["expected"]
            self.case_files[name] = files
        legacy = (
            "[playlist]\nFile1=https://media.example.invalid/legacy.mp3\nTitle1=Äther\n"
        )
        self.legacy = legacy.encode(charset)
        config = ET.parse(self.config)
        ns = config.getroot().tag.split("}")[0] + "}"
        config.find(".//" + ns + "playlist-charset").text = charset
        if args.exercise_media_updates:
            config.find(".//" + ns + "autoscan").set("use-inotify", "yes")
            directory = config.find(".//" + ns + "autoscan/" + ns + "directory")
            directory.set("mode", "inotify")
        config.write(self.config, encoding="utf-8", xml_declaration=True)
        self.original_hashes = self.hash_sources()

    def start(self):
        super().start()
        if self.args.exercise_media_updates:
            # The inherited XML fixture checks lifecycle readiness. This
            # inotify scenario also needs watches on the directories it edits.
            self.server.wait_for_watches(
                (
                    self.src,
                    self.src / "f01",
                    self.case_files["f01"]["A"].parent,
                    self.src / "f15",
                )
            )

    def media_indexed(self):
        expected = sum(len(files) for files in self.case_files.values())
        pending = ["1"]  # Gerbera's filesystem root, before adding playlists.
        count = 0
        while pending:
            for node in self.browse(pending.pop()):
                if node.tag.endswith("}container"):
                    pending.append(node.get("id"))
                elif node.findtext(UP + "class", "").startswith("object.item"):
                    count += 1
        return count >= expected

    def add_playlists(self):
        for path, contents in self.pending_playlists.items():
            path.write_bytes(contents)
        self.original_hashes = self.hash_sources()

    def assert_views(self):
        found = self.playlist_views()
        assert not (set(found) - set(self.expected)), set(found) - set(self.expected)
        for name, expected in self.expected.items():
            views = found.get(name, [])
            if name in self.deleted:
                assert not views, (name, "deleted playlist still has containers")
                continue
            assert len(views) == 2 or (not expected and not views), (name, len(views))
            for view in views:
                items = self.browse(view.get("id"))
                assert len(items) == len(expected), (name, len(items), expected)
                for index, (item, key) in enumerate(zip(items, expected)):
                    resource = next(n for n in item if n.tag.endswith("}res"))
                    if key == "HTTPS":
                        assert resource.text == self.remote_media, (name, resource.text)
                        continue
                    if key == "LEGACY":
                        assert (
                            resource.text == "https://media.example.invalid/legacy.mp3"
                        )
                        assert item.findtext(DC + "title") == "Äther"
                        continue
                    media = self.manifest["media"][key]
                    title = "v" if key == "V" else media["title"]
                    if name == "f35" and index == 0:
                        title = "Override"
                    title = self.title_overrides.get((name, key), title)
                    assert item.findtext(DC + "title") == title, (
                        name,
                        key,
                        item.findtext(DC + "title"),
                        title,
                    )
                    cls = item.findtext(UP + "class")
                    assert cls.startswith(
                        "object.item.videoItem"
                        if key == "V"
                        else "object.item.audioItem"
                    ), (name, cls)
                    assert resource.text.startswith(self.host + "/"), (
                        name,
                        resource.text,
                    )
                    with HTTP.open(resource.text, timeout=5) as response:
                        data = response.read()
                    assert data == self.case_files[name][key].read_bytes(), (name, key)
                    if name == "f25":
                        assert item.findtext(UP + "author") == (
                            "Entry author" if index == 0 else "Root author"
                        ), ET.tostring(item)
                        assert (
                            item.findtext(DC + "rights") == "Root rights"
                        ), ET.tostring(item)
                        if index == 0:
                            assert (
                                item.findtext(DC + "description") == "Entry description"
                            ), ET.tostring(item)
                    if name == "f27":
                        duration = resource.get("duration")
                        assert duration, resource.attrib
                        hours, minutes, seconds = duration.split(":")
                        actual = int(hours) * 3600 + int(minutes) * 60 + float(seconds)
                        assert abs(actual - 0.1) < 0.01, resource.attrib
                    self.resource_checks += 1
                self.view_checks += 1
        assert self.hash_sources() == self.original_hashes
        assert not self.remote_requests, self.remote_requests
        return True

    def run(self):
        try:
            if self.args.exercise_media_updates:
                self.add_playlists()
            self.start()
            if not self.args.exercise_media_updates:
                # Keep this PR's semantic test independent of the separate
                # concurrent initial-import/source-identity repair (#4006).
                self.wait(self.media_indexed)
                self.add_playlists()
                self.rescan()
            self.record(
                "initial structures, exact ordered positions, metadata and complete resources"
            )
            (self.src / "zlegacy.pls").write_bytes(self.legacy)
            self.expected["zlegacy"] = ["LEGACY"]
            self.original_hashes = self.hash_sources()
            self.rescan()
            self.record("new legacy import after XML restores its configured charset")
            if not self.args.exercise_media_updates:
                self.rescan()
                self.record("unchanged rescan retains all semantic positions")
                self.stop()
                self.start()
                self.record("restart retains all semantic positions")
                return self.result()
            case = self.src / "f01/f01.asx"
            case.write_text(
                '<ASX><ENTRY><REF HREF="media/b.wav"/></ENTRY><REPEAT COUNT="2"><ENTRY><REF HREF="media/a.wav"/></ENTRY></REPEAT></ASX>'
            )
            self.expected["f01"] = ["B", "A", "A"]
            self.original_hashes = self.hash_sources()
            self.record("ordinary parent playlist edit through inotify")
            media = self.case_files["f01"]["A"]
            media.write_bytes(wav_bytes("Changed source title"))
            self.title_overrides[("f01", "A")] = "Changed source title"
            self.original_hashes = self.hash_sources()
            self.record(
                "ordinary media edit preserves repeated playlist references through inotify"
            )
            with media.open("r+b"):
                pass
            time.sleep(0.5)
            self.record("unchanged writable media close retains expanded positions")
            child = self.src / "f15/.sub/child.asx"
            child.write_text(
                '<ASX><ENTRY><REF HREF="../media/c.wav"/></ENTRY><ENTRY><REF HREF="../media/b.wav"/></ENTRY></ASX>'
            )
            self.expected["f15"] = ["A", "C", "B", "A"]
            # Gerbera tracks playlist mtimes, not nested-document dependencies.
            time.sleep(1.1)
            (self.src / "f15/f15.asx").touch()
            self.original_hashes = self.hash_sources()
            self.rescan()
            self.record("child edit refreshed through parent modification and rescan")
            self.stop()
            self.start()
            self.record("restart retains expanded lists and metadata")
            case.write_text("<ASX/>")
            self.expected["f01"] = []
            self.original_hashes = self.hash_sources()
            self.rescan()
            self.record("empty parent removes obsolete positions")
            case.unlink()
            self.deleted.add("f01")
            self.original_hashes = self.hash_sources()
            self.rescan()
            self.record("actual parent deletion keeps other lists intact")
            return self.result()
        finally:
            self.stop()
            self.remote.shutdown()
            self.remote.server_close()
            self.remote_thread.join()

    def result(self):
        return {
            "stages": self.checks,
            "documents": len(self.manifest["cases"]),
            "view_checks": self.view_checks,
            "resource_checks": self.resource_checks,
            "unexpected_remote_requests": self.remote_requests,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--ip")
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--exercise-media-updates",
        action="store_true",
        help="Combined regression with the independent playlist-update repair (#4006)",
    )
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.source_root = args.source_root.resolve()
    results = {}
    with fixture_workspace(None, prefix="grb-af-") as tmp:
        for charset in ["UTF-8", "windows-1252"]:
            for mode in ["mt", "grb"]:
                root = Path(tmp) / (mode + "-" + charset)
                root.mkdir()
                results[root.name] = AsxSemanticsFixture(
                    args, root, mode, charset
                ).run()
    if args.output:
        args.output.write_text(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
