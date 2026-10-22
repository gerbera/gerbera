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
"""Exercise ASX URI and charset regressions through a real ContentDirectory."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import xml.etree.ElementTree as ET

from test_xml_playlists import DC, UP, HTTP, Fixture, wav_bytes


class AsxUriFixture(Fixture):
    def __init__(self, args, root, mode, charset):
        super().__init__(args, root, mode)
        # Replace only this newly generated, isolated fixture's source tree.
        shutil.rmtree(self.src)
        self.src.mkdir()
        corpus = args.source_root / "test/scripting/fixtures/asx-uris"
        self.manifest = json.loads((corpus / "cases.json").read_text())
        self.expected = {}
        self.case_files = {}
        self.checks = []
        self.view_checks = 0
        self.resource_checks = 0
        for case in self.manifest["cases"]:
            name = case["id"]
            directory = self.src / name
            directory.mkdir()
            files = {}
            for key, media in self.manifest["media"].items():
                if "path" not in media or key in case.get("absent_media", []):
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
            data = (
                data.decode(encoding)
                .replace("/fixture/asx", str(directory))
                .encode(encoding)
            )
            (directory / case["file"]).write_bytes(data)
            self.expected[name] = case["expected"]
            self.case_files[name] = files
        self.external = {}
        for name, uri, title in [
            ("zutfxml", "https://media.example.invalid/xml.mp3", "Äther 東京"),
            ("zmms", "mms://media.example.invalid/a.wmv?token=a+b", "Stream"),
            ("zrtsp", "rtsp://media.example.invalid/a.wmv?token=Why%3F", "Stream"),
        ]:
            document = ET.Element("ASX", VERSION="3.0")
            entry = ET.SubElement(document, "ENTRY")
            ET.SubElement(entry, "TITLE").text = title
            ET.SubElement(entry, "REF", HREF=uri)
            ET.ElementTree(document).write(
                self.src / (name + ".asx"), encoding="utf-8", xml_declaration=True
            )
            self.expected[name] = ["EXTERNAL"]
            self.external[name] = (uri, title)
        # A legacy parser invocation after XML must still use its configured charset.
        legacy = "[playlist]\nFile1=https://media.example.invalid/legacy.mp3\nTitle1=Ã„ther\n"
        (self.src / "zlegacy.pls").write_bytes(legacy.encode(charset))
        self.expected["zlegacy"] = ["LEGACY"]
        config = ET.parse(self.config)
        ns = config.getroot().tag.split("}")[0] + "}"
        config.find(".//" + ns + "playlist-charset").text = charset
        config.write(self.config, encoding="utf-8", xml_declaration=True)
        self.original_hashes = self.hash_sources()

    def assert_views(self):
        found = self.playlist_views()
        assert not (set(found) - set(self.expected)), set(found) - set(self.expected)
        for name, expected in self.expected.items():
            views = found.get(name, [])
            assert len(views) == 2 or (not expected and not views), (name, len(views))
            for view in views:
                items = self.browse(view.get("id"))
                assert len(items) == len(expected), (
                    name,
                    [i.findtext(DC + "title") for i in items],
                    expected,
                )
                for item, key in zip(items, expected):
                    resource = next(n for n in item if n.tag.endswith("}res"))
                    if key == "EXTERNAL":
                        uri, title = self.external[name]
                        assert resource.text == uri
                        assert item.findtext(DC + "title") == title
                        continue
                    if key == "HTTPS":
                        assert resource.text == self.manifest["media"][key]["uri"]
                        continue  # Never probe remote media or playlist references.
                    if key == "LEGACY":
                        assert (
                            resource.text == "https://media.example.invalid/legacy.mp3"
                        )
                        assert item.findtext(DC + "title") == "Ã„ther"
                        continue
                    media = self.manifest["media"][key]
                    title = "v" if key == "V" else media["title"]
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
                    self.resource_checks += 1
                self.view_checks += 1
        assert self.hash_sources() == self.original_hashes
        return True

    def run(self):
        try:
            self.start()
            self.record(
                "ASX references, exact positions, UTF-8 metadata and complete HTTP resources"
            )
            # Import this PLS only after all initial XML imports have completed.
            after = self.src / "zafter.pls"
            after.write_bytes((self.src / "zlegacy.pls").read_bytes())
            self.expected["zafter"] = ["LEGACY"]
            self.original_hashes["zafter.pls"] = hashlib.sha256(
                after.read_bytes()
            ).hexdigest()
            self.rescan()
            self.record("rescan and subsequent legacy converter reuse")
            self.stop()
            self.start()
            self.record("restart preserves both playlist views")
            return {
                "stages": self.checks,
                "documents": len(self.manifest["cases"]),
                "view_checks": self.view_checks,
                "resource_checks": self.resource_checks,
            }
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
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.source_root = args.source_root.resolve()
    results = {}
    with tempfile.TemporaryDirectory(prefix="grb-au-") as tmp:
        for charset in ["UTF-8", "windows-1252"]:
            for mode in ["mt", "grb"]:
                root = Path(tmp) / (mode + "-" + charset)
                root.mkdir()
                results[root.name] = AsxUriFixture(args, root, mode, charset).run()
    if args.output:
        args.output.write_text(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
