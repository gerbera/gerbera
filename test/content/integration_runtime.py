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

"""Shared lifecycle and HTTP support for isolated Gerbera integration tests."""

import contextlib
import http.client
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import urllib.request
import xml.etree.ElementTree as ET


class FixtureError(RuntimeError):
    """An owned process failure or an unmet condition at its deadline."""


class _HTTPConnection(http.client.HTTPConnection):
    def connect(self):
        # Bind the client first, excluding the destination port before connect.
        # An unbound client connecting to a local, not-yet-listening server can
        # otherwise receive that same ephemeral port and connect to itself.
        address = socket.getaddrinfo(self.host, self.port, socket.AF_INET,
                                     socket.SOCK_STREAM)[0][4]
        for _ in range(16):
            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                client.settimeout(self.timeout)
                client.bind(("", 0))
                if client.getsockname()[1] == address[1]:
                    client.close()
                    continue
                client.connect(address)
                self.sock = client
                return
            except BaseException:
                client.close()
                raise
        raise FixtureError("Cannot allocate a client port distinct from the server")


class _HTTPHandler(urllib.request.HTTPHandler):
    def http_open(self, request):
        return self.do_open(_HTTPConnection, request)


HTTP = urllib.request.build_opener(urllib.request.ProxyHandler({}), _HTTPHandler())


def wait_for(check, timeout=20, *, alive=None):
    """Preserve playlist convergence retries within one finite deadline."""
    if not math.isfinite(timeout) or timeout <= 0:
        raise ValueError("The wait timeout must be finite and positive")
    deadline = time.monotonic() + timeout
    last = None
    while True:
        if alive:
            alive()
        try:
            value = check()
            if alive:
                alive()
            if value:
                return value
        except (OSError, ValueError, AssertionError, ET.ParseError, http.client.IncompleteRead) as error:
            last = str(error)
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise FixtureError(f"Condition did not converge within {timeout}s: {last}")
        time.sleep(min(0.05, remaining))


class GerberaProcess:
    """One owned process; every start has fresh logs and a server-owned port."""

    def __init__(self, binary, config, ip, directory, *, debug=False):
        self.binary = Path(binary)
        self.config = Path(config)
        self.ip = ip
        self.directory = Path(directory)
        self.debug = debug
        self.process = None
        self.host = None
        self.log = None
        self.log_path = None
        self.generation = 0

    def check_alive(self):
        if self.process is None or self.process.poll() is not None:
            code = None if self.process is None else self.process.returncode
            raise FixtureError(self.diagnostic(f"Gerbera exited (status {code})"))

    def diagnostic(self, reason):
        tail = ""
        if self.log_path and self.log_path.exists():
            with self.log_path.open("rb") as stream:
                stream.seek(max(0, self.log_path.stat().st_size - 8000))
                tail = stream.read().decode(errors="replace")
        return f"{reason}\nEndpoint: {self.host}\nLog: {self.log_path}\n{tail}"

    def _ready(self):
        self.check_alive()
        text = self.log_path.read_text(errors="replace")
        # A bound socket is not readiness. Use the existing lifecycle event
        # published after server->init() AND server->run(), including restarts.
        if "Service status: READY=1" not in text:
            return False
        match = re.search(r"IPv4: Server bound to: ([0-9.]+):([0-9]+)", text)
        if not match:
            raise FixtureError("READY=1 without an IPv4 endpoint")
        self.host = f"http://{match[1]}:{int(match[2])}"
        return True

    def start(self, ready_check, *, timeout=30):
        if self.process is not None:
            raise FixtureError("Stop the owned process before starting it again")
        self.generation += 1
        self.host = None
        self.log_path = self.directory / f"server-{self.generation:04d}.log"
        self.log = self.log_path.open("wb")
        command = [str(self.binary), "-c", str(self.config), "-i", self.ip, "-p", "0"]
        if self.debug:
            command += ["--debug", "--debug-mode", "autoscan,content,layout"]
        try:
            self.process = subprocess.Popen(command, stdout=self.log,
                                            stderr=subprocess.STDOUT,
                                            start_new_session=(os.name == "posix"))
            wait_for(self._ready, timeout, alive=self.check_alive)
            # Confirm this fixture's identity, not an unrelated listener.
            config = ET.parse(self.config).getroot()
            expected = config.findtext(".//{*}udn")
            with HTTP.open(self.host + "/upnp/description.xml", timeout=5) as response:
                actual = ET.fromstring(response.read()).findtext(".//{*}UDN")
            if not expected or actual != expected:
                raise FixtureError(f"Wrong server identity: {actual!r}, expected {expected!r}")
            ready_check()
            self.check_alive()
        except BaseException as error:
            print(self.diagnostic(f"Start failed: {error}"), flush=True)
            try:
                self.stop()
            except Exception as cleanup:
                raise FixtureError(
                    f"Start failed: {error}\nCleanup also failed: {str(cleanup).splitlines()[0]}"
                ) from error
            raise

    def wait(self, check, timeout=20):
        try:
            return wait_for(check, timeout, alive=self.check_alive)
        except Exception as error:
            raise FixtureError(self.diagnostic(str(error))) from error

    def wait_for_watches(self, paths, timeout=20):
        """Linux: require the kernel watches this test needs, not global idle."""
        paths = tuple(Path(path) for path in paths)
        # IN_CLOSE_WRITE, IN_MOVED_FROM/TO, IN_CREATE and IN_DELETE.
        required_mask = 0x8 | 0x40 | 0x80 | 0x100 | 0x200

        def registered():
            expected = {}
            for path in paths:
                info = path.stat()
                device = (os.major(info.st_dev) << 20) | os.minor(info.st_dev)
                expected[device, info.st_ino] = str(path)
            found = set()
            for fd in Path(f"/proc/{self.process.pid}/fdinfo").iterdir():
                try:
                    text = fd.read_text()
                except FileNotFoundError:
                    continue  # An owned worker closed the descriptor.
                for inode, device, mask in re.findall(
                    r"inotify wd:[0-9a-f]+ ino:([0-9a-f]+) sdev:([0-9a-f]+) mask:([0-9a-f]+)", text
                ):
                    if int(mask, 16) & required_mask == required_mask:
                        found.add((int(device, 16), int(inode, 16)))
            missing = [path for key, path in expected.items() if key not in found]
            assert not missing, f"Inotify watches not registered: {missing}"
            return True

        self.wait(registered, timeout)

    def _signal(self, sig):
        try:
            if os.name == "posix":
                os.killpg(self.process.pid, sig)
            elif self.process.poll() is None:
                self.process.send_signal(sig)
        except ProcessLookupError:
            pass

    def stop(self, timeout=15):
        if self.process is None:
            if self.log:
                self.log.close()
            return
        forced = False
        try:
            self._signal(signal.SIGTERM)
            try:
                self.process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                forced = True
                self._signal(signal.SIGKILL)
                self.process.wait(timeout=5)
        finally:
            if os.name == "posix":
                self._signal(signal.SIGKILL)  # Also reap fixture-owned descendants.
            self.log.close()
            self.process = None
        if forced:
            raise FixtureError(self.diagnostic("Gerbera did not stop gracefully"))


@contextlib.contextmanager
def fixture_workspace(path=None, *, prefix="grb-test-"):
    """Remove owned files after success; retain logs/config/DB on every failure."""
    owned = path is None
    if owned:
        artifacts = os.environ.get("GRB_TEST_ARTIFACTS")
        if artifacts:
            Path(artifacts).mkdir(parents=True, exist_ok=True)
        root = Path(tempfile.mkdtemp(prefix=prefix, dir=artifacts)).resolve()
    else:
        root = Path(path).resolve()
        root.mkdir(parents=True, exist_ok=False)
    try:
        yield root
    except BaseException as error:
        (root / "failure.json").write_text(json.dumps({"error": str(error)}, indent=2))
        print(f"Integration failure evidence retained: {root}", flush=True)
        for log in sorted(root.rglob("server-*.log")):
            with log.open("rb") as stream:
                stream.seek(max(0, log.stat().st_size - 8000))
                print(f"--- {log} ---\n{stream.read().decode(errors='replace')}", flush=True)
        raise
    else:
        if owned:
            shutil.rmtree(root)
