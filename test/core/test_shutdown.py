#!/usr/bin/env python3
# Gerbera - https://gerbera.io/
# Copyright (C) 2026 Gerbera Contributors
# SPDX-License-Identifier: GPL-2.0-only
"""A SIGTERM sent while Gerbera is still starting must stop it."""

import argparse
import signal
import socket
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from pathlib import Path


def local_ip():
    try:
        ip = socket.gethostbyname(socket.gethostname())
    except OSError:
        ip = "127.0.0.1"
    if not ip.startswith("127."):
        return ip
    # libupnp refuses loopback: take the address of the default route instead
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
            probe.connect(("192.0.2.1", 9))
            return probe.getsockname()[0]
    except OSError:
        return ip


def free_port(ip):
    with socket.socket() as probe:
        probe.bind((ip, 0))
        return probe.getsockname()[1]


def write_config(binary, source_root, root):
    xml = subprocess.check_output([str(binary), "--create-config"]).decode()
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

    web = root / "web"
    web.mkdir()
    set_value("server/home", root)
    set_value("server/webroot", web)
    set_value("server/storage/sqlite3/database-file", root / "db.sqlite")
    for tag, name in (
        ("init-sql-file", "sqlite3.sql"),
        ("upgrade-file", "sqlite3-upgrade.xml"),
        ("drop-file", "sqlite3-drop.sql"),
    ):
        set_value(
            "server/storage/sqlite3/" + tag,
            source_root / "src/database/sqlite3" / name,
        )
    set_value("server/storage/sqlite3/backup", "").set("enabled", "no")
    set_value("server/ui", "").set("enabled", "yes")
    set_value("server/ui/accounts", "").set("enabled", "no")
    set_value("import/filesystem-charset", "UTF-8")
    set_value("import/scripting/script-folder/common", source_root / "scripts/js")
    path = root / "config.xml"
    ET.ElementTree(config).write(path, encoding="utf-8", xml_declaration=True)
    return path


def wait_for_port(ip, port, process, timeout=60):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if process.poll() is not None:
            return False
        try:
            socket.create_connection((ip, port), timeout=1).close()
            return True
        except OSError:
            time.sleep(0.02)
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--ip")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--timeout", type=float, default=20)
    args = parser.parse_args()

    ip = args.ip or local_ip()
    failed = []
    for number in range(1, args.rounds + 1):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            config = write_config(args.binary, args.source_root, root)
            port = free_port(ip)
            log = (root / "server.log").open("wb")
            process = subprocess.Popen(
                [str(args.binary), "-c", str(config), "-i", ip, "-p", str(port)],
                stdout=log,
                stderr=subprocess.STDOUT,
            )
            try:
                if not wait_for_port(ip, port, process):
                    status = process.poll()
                    print(f"round {number}: Gerbera did not start, status {status}")
                    print((root / "server.log").read_text(errors="replace")[-2000:])
                    failed.append(number)
                    continue
                # the web server listens before main() has finished starting
                process.send_signal(signal.SIGTERM)
                try:
                    process.wait(timeout=args.timeout)
                    print(f"round {number}: stopped, status {process.returncode}")
                except subprocess.TimeoutExpired:
                    print(
                        f"round {number}: not stopped {args.timeout:g} s after SIGTERM"
                    )
                    failed.append(number)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                log.close()
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
