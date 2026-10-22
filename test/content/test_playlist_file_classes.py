#!/usr/bin/env python3
# Gerbera - https://gerbera.io/
# Copyright (C) 2026 Gerbera Contributors
# SPDX-License-Identifier: GPL-2.0-only
"""Check physical playlist classes and preserve parsed playlist contents."""

import argparse
import hashlib
import json
from pathlib import Path
import tempfile
import xml.etree.ElementTree as ET

from test_playlist_updates import DC, UP, Fixture, wait_for


PLAYLIST = "object.item.playlistItem"
TRACK = "object.item.audioItem.musicTrack"
BROADCAST = "object.item.audioItem.audioBroadcast"
MOVIE = "object.item.videoItem.movie"


def run_case(args, root, mode, case, xml_types):
    fixture = Fixture(args, root, mode, "timed")
    if xml_types:
        paths = ["../audio/" + fixture.files[key].name for key in fixture.order]
        fixture.names.update(five="wpl", six="xspf")
        wpl = ET.Element("smil")
        seq = ET.SubElement(ET.SubElement(wpl, "body"), "seq")
        xspf = ET.Element("playlist", xmlns="http://xspf.org/ns/0/", version="1")
        tracks = ET.SubElement(xspf, "trackList")
        for path in paths:
            ET.SubElement(seq, "media", src=path)
            ET.SubElement(ET.SubElement(tracks, "track"), "location").text = path
        ET.ElementTree(wpl).write(fixture.playlists / "five.wpl", encoding="utf-8")
        ET.ElementTree(xspf).write(fixture.playlists / "six.xspf", encoding="utf-8")

    config = ET.parse(fixture.config)
    ns = config.getroot().tag.split("}")[0] + "}"
    mappings = config.find(f"{ns}import/{ns}mappings/{ns}mimetype-upnpclass")
    assert mappings is not None
    expected = dict.fromkeys(fixture.names, PLAYLIST)
    if case in ("legacy", "disabled-defaults"):
        mappings.clear()
        mappings.set("extend", "yes")
        for mime, cls in [("audio/*", TRACK), ("video/*", "object.item.videoItem")]:
            ET.SubElement(mappings, ns + "map", {"from": mime, "to": cls})
    if case == "disabled-defaults":
        mappings.set("extend", "no")
        expected.update(one=TRACK, two=TRACK, three=TRACK, four="object.item.videoItem")
        if xml_types:
            expected.update(five="object.item", six="object.item")
    if case in ("exact-override", "conditional-override"):
        key = "audio/x-mpegurl" if case == "exact-override" else "audio/*;location=one"
        # Replace an existing generated default, rather than add duplicate keys.
        for child in list(mappings):
            if child.get("from") == key:
                mappings.remove(child)
        ET.SubElement(mappings, ns + "map", {"from": key, "to": BROADCAST})
        expected["one"] = BROADCAST
        if case == "exact-override":
            expected["two"] = BROADCAST
            for child in list(mappings):
                if child.get("from") == "video/x-ms-asx":
                    mappings.remove(child)
            ET.SubElement(mappings, ns + "map", {"from": "video/x-ms-asx", "to": MOVIE})
            expected["four"] = MOVIE
    config.write(fixture.config, encoding="utf-8", xml_declaration=True)
    before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixture.src.rglob("*") if p.is_file()}
    try:
        fixture.start()
        wait_for(fixture.assert_views)
        physical = fixture.search(2, 'upnp:class derivedfrom "object.item"')
        items = {n.findtext(DC + "title"): n for n in physical}
        for name, cls in expected.items():
            node = items[name]
            assert node.tag.endswith("}item"), (name, node.tag)
            assert node.findtext(UP + "class") == cls, (mode, case, name, node.findtext(UP + "class"), cls)
            resource = next(n for n in node if n.tag.endswith("}res"))
            mime = resource.get("protocolInfo").split(":", 3)[2]
            wanted = {"m3u": "audio/x-mpegurl", "m3u8": "audio/x-mpegurl", "pls": "audio/x-scpls",
                      "asx": "video/x-ms-asx", "wpl": "application/vnd.ms-wpl", "xspf": "application/xspf+xml"}
            assert mime == wanted[fixture.names[name]], (name, mime)
            assert int(resource.get("size")) == (fixture.playlists / (name + "." + fixture.names[name])).stat().st_size
        for title in fixture.titles.values():
            assert items[title].findtext(UP + "class") == TRACK, title
        assert all(hashlib.sha256(p.read_bytes()).hexdigest() == value for p, value in before.items())
        return {"mode": mode, "case": case, "physical_classes": expected,
                "formats": list(fixture.names.values()), "music_unchanged": True,
                "virtual_order_and_repeated_entries_preserved": True}
    finally:
        fixture.stop()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--ip")
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--xml-playlists", action="store_true", help="Require the WPL/XSPF parser integration")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    # Include XML formats automatically once their parser is available; the
    # explicit flag makes integration checks fail if that support is missing.
    xml_types = "function readXmlPlaylist(" in (args.source_root / "scripts/js/playlists.js").read_text()
    assert not args.xml_playlists or xml_types, "WPL/XSPF parser support missing"
    results = []
    with tempfile.TemporaryDirectory(prefix="grb-pclass-") as directory:
        for mode in ("mt", "grb"):
            for case in ("default", "legacy", "exact-override", "conditional-override", "disabled-defaults"):
                root = Path(directory) / (mode + "-" + case)
                root.mkdir()
                result = run_case(args, root, mode, case, xml_types)
                results.append(result)
                print(json.dumps(result), flush=True)
    if args.output:
        args.output.write_text(json.dumps(results, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
