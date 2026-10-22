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
"""Check prepared ASX input data; this does not run Gerbera."""

import argparse
import hashlib
import json
from pathlib import Path
import urllib.parse
import xml.etree.ElementTree as ET


def verify(folder):
    folder = folder.resolve()
    manifest = json.loads((folder / "cases.json").read_text(encoding="utf-8"))
    ids = set()
    names = set()
    documents = []
    decisions = []
    valid = invalid = 0
    for case in manifest["cases"]:
        assert case["id"] not in ids, case["id"]
        assert case["file"] not in names, case["file"]
        ids.add(case["id"])
        names.add(case["file"])
        target = folder / case["file"]
        assert target.resolve().is_relative_to(folder), target
        data = target.read_bytes()
        assert hashlib.sha256(data).hexdigest() == case["sha256"], case["file"]
        expected = case["expected"]
        if expected is None:
            assert case["expectation"] == "requires_decision" and case["decision"]
            decisions.append(case["id"])
        else:
            assert isinstance(expected, list)
            assert all(key in manifest["media"] for key in expected), case["id"]
        for availability in ["present_media", "absent_media"]:
            assert all(key in manifest["media"] for key in case.get(availability, []))
        try:
            root = ET.fromstring(data)
        except ET.ParseError:
            assert case["xml"] == "invalid" and case["outcome"] == "reject", case["id"]
            invalid += 1
        else:
            assert case["xml"] == "valid" and root.tag.lower() == "asx", case["id"]
            valid += 1
            documents.append((target, root))
    for name, digest in manifest.get("dependencies", {}).items():
        target = folder / name
        assert target.resolve().is_relative_to(folder), target
        data = target.read_bytes()
        assert hashlib.sha256(data).hexdigest() == digest, name
        root = ET.fromstring(data)
        assert root.tag.lower() == "asx"
        documents.append((target, root))
    for target, root in documents:
        for node in root.iter():
            if node.tag.lower() != "entryref":
                continue
            attrs = {key.lower(): value for key, value in node.attrib.items()}
            href = attrs["href"]
            if urllib.parse.urlsplit(href).scheme:
                continue
            nested = (target.parent / urllib.parse.unquote(href)).resolve()
            assert nested.is_relative_to(folder), (target, href)
            if nested.name != "missing.asx":
                assert nested.is_file(), (target, href)
    return {
        "issue": manifest["issue"],
        "cases": len(ids),
        "valid_xml": valid,
        "intentionally_invalid_xml": invalid,
        "child_documents": len(manifest.get("dependencies", {})),
        "policy_decisions_explicitly_unresolved": decisions,
        "xml_hashes_manifest_and_local_child_links": "passed",
        "gerbera_importer_executed": False,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path)
    args = parser.parse_args()
    print(json.dumps(verify(args.folder), indent=2))
