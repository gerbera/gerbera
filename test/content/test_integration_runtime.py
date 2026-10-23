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

"""Regression tests for lifecycle races, independent of a Gerbera installation."""

import contextlib
import ctypes
import http.client
from http.server import BaseHTTPRequestHandler, HTTPServer
import io
import os
from pathlib import Path
import signal
import shlex
import socket
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import Mock, patch
import urllib.error
import xml.etree.ElementTree as ET

from integration_runtime import (
    HTTP, FixtureError, GerberaProcess, _HTTPConnection, fixture_workspace, wait_for,
)
from test_playlist_updates import Fixture


class DescriptionHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        body = ('<root xmlns="urn:schemas-upnp-org:device-1-0"><device>'
                f'<UDN>{self.server.udn}</UDN></device></root>').encode()
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if getattr(self.server, "short_responses", 0):
            self.server.short_responses -= 1
            body = body[:5]
            self.close_connection = True
        self.wfile.write(body)

    def log_message(self, format, *args):
        pass


def child():
    config = ET.parse(sys.argv[sys.argv.index("-c") + 1]).getroot()
    ip = sys.argv[sys.argv.index("-i") + 1]
    mode = config.findtext("mode", "ready")
    # HTTPServer's reverse-DNS lookup is irrelevant to this numeric-IP fixture.
    with patch("socket.getfqdn", return_value=ip):
        listener = HTTPServer((ip, 0), DescriptionHandler)
    with listener as server:
        server.udn = config.findtext("actual-udn", config.findtext("server/udn"))
        print(f"IPv4: Server bound to: {ip}:{server.server_port}", flush=True)
        if mode == "exit":
            sys.exit(7)
        if mode == "never-ready":
            while True:
                time.sleep(1)
        delay = float(config.findtext("delay", "0"))
        time.sleep(delay)
        if mode == "ignore-term":
            signal.signal(signal.SIGTERM, signal.SIG_IGN)
        print("Service status: READY=1", flush=True)
        server.serve_forever()


class TransportTest(unittest.TestCase):
    def test_destination_port_is_rejected_before_connect(self):
        first, second = Mock(), Mock()
        first.getsockname.return_value = ("0.0.0.0", 49170)
        second.getsockname.return_value = ("0.0.0.0", 40000)
        with patch("integration_runtime.socket.socket", side_effect=[first, second]):
            connection = _HTTPConnection("127.0.0.1", 49170, timeout=1)
            connection.connect()
        first.connect.assert_not_called()
        first.close.assert_called_once()
        second.connect.assert_called_once_with(("127.0.0.1", 49170))
        connection.close()

    def test_real_refused_connection_does_not_echo_its_request(self):
        with socket.socket() as guard:
            guard.bind(("127.0.0.1", 0))
            port = guard.getsockname()[1]
            with self.assertRaises(urllib.error.URLError):
                HTTP.open(f"http://127.0.0.1:{port}/", timeout=0.5)

    def test_legacy_retryable_conditions_can_converge(self):
        errors = [AssertionError("pending"), OSError("pending"), ValueError("pending"),
                  ET.ParseError("pending"), urllib.error.URLError("pending")]
        for error in errors:
            with self.subTest(error=type(error).__name__):
                check = Mock(side_effect=[error, "done"])
                self.assertEqual(wait_for(check, 1), "done")
                self.assertEqual(check.call_count, 2)

    def test_short_http_body_recovers_but_persistent_short_reads_fail(self):
        for short_responses in (1, 100):
            with self.subTest(short_responses=short_responses):
                with patch("socket.getfqdn", return_value="127.0.0.1"):
                    listener = HTTPServer(("127.0.0.1", 0), DescriptionHandler)
                with listener as server:
                    server.udn = "uuid:short-read"
                    server.short_responses = short_responses
                    thread = threading.Thread(target=server.serve_forever)
                    thread.start()
                    try:
                        def read():
                            with HTTP.open(f"http://127.0.0.1:{server.server_port}/", timeout=1) as response:
                                return response.read()

                        if short_responses == 1:
                            body = wait_for(read, timeout=1)
                            self.assertIn(b"<UDN>uuid:short-read</UDN>", body)
                            self.assertEqual(server.short_responses, 0)
                        else:
                            with self.assertRaisesRegex(FixtureError, "IncompleteRead"):
                                wait_for(read, timeout=0.15)
                            self.assertLess(server.short_responses, short_responses)
                    finally:
                        server.shutdown()
                        thread.join(timeout=2)
                        self.assertFalse(thread.is_alive())

    def test_programming_errors_are_not_retried(self):
        for error in [TypeError("unexpected"), RuntimeError("unexpected"),
                      http.client.BadStatusLine("invalid status")]:
            with self.subTest(error=type(error).__name__):
                check = Mock(side_effect=error)
                with self.assertRaises(type(error)):
                    wait_for(check, 1)
                self.assertEqual(check.call_count, 1)

    def test_timeout_is_bounded_and_reports_last_assertion(self):
        started = time.monotonic()
        with self.assertRaisesRegex(FixtureError, "pending"):
            wait_for(Mock(side_effect=AssertionError("pending")), 0.1)
        self.assertLess(time.monotonic() - started, 1)

    def test_invalid_deadlines_are_rejected(self):
        for timeout in (0, -1, float("nan"), float("inf")):
            with self.subTest(timeout=timeout):
                with self.assertRaises(ValueError):
                    wait_for(lambda: True, timeout)


class CatalogConvergenceTest(unittest.TestCase):
    def setUp(self):
        self.fixture = Fixture.__new__(Fixture)
        self.fixture.server = Mock(host="http://127.0.0.1:12345")

    @staticmethod
    def fault(code=701, status=500):
        body = ('<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">'
                '<s:Body><s:Fault><detail>'
                '<UPnPError xmlns="urn:schemas-upnp-org:control-1-0">'
                f'<errorCode>{code}</errorCode></UPnPError>'
                '</detail></s:Fault></s:Body></s:Envelope>').encode()
        return urllib.error.HTTPError("http://127.0.0.1:12345", status, "fault", {}, io.BytesIO(body))

    def test_legacy_http_faults_can_converge(self):
        response = b'<Envelope><Body><Result>ready</Result></Body></Envelope>'
        for error in [self.fault(701), self.fault(501), self.fault(402),
                      self.fault(status=404), urllib.error.URLError("connection failed")]:
            with self.subTest(error=str(error)):
                with patch.object(HTTP, "open", side_effect=[error, io.BytesIO(response)]) as opened:
                    result = wait_for(lambda: self.fixture.soap_response(
                        "Browse", {"ObjectID": "42"}).findtext(".//Result"), timeout=1)
                self.assertEqual(result, "ready")
                self.assertEqual(opened.call_count, 2)

    def test_persistent_faults_fail_at_deadline_with_last_error(self):
        for error in [self.fault(501), self.fault(status=404),
                      urllib.error.URLError("connection failed")]:
            with self.subTest(error=str(error)):
                with patch.object(HTTP, "open", side_effect=error) as opened:
                    with self.assertRaises(FixtureError) as result:
                        wait_for(lambda: self.fixture.soap_response(
                            "Browse", {"ObjectID": "42"}), timeout=0.1)
                self.assertIn(str(error), str(result.exception))
                self.assertGreater(opened.call_count, 1)

    def test_partial_xml_can_converge_but_permanent_bad_xml_fails(self):
        response = b'<Envelope><Body><Result>ready</Result></Body></Envelope>'
        with patch.object(HTTP, "open", side_effect=[io.BytesIO(b"not XML"), io.BytesIO(response)]):
            result = wait_for(lambda: self.fixture.soap_response(
                "Browse", {"ObjectID": "42"}).findtext(".//Result"), timeout=1)
        self.assertEqual(result, "ready")
        with patch.object(HTTP, "open", side_effect=lambda *a, **k: io.BytesIO(b"not XML")):
            with self.assertRaises(FixtureError):
                wait_for(lambda: self.fixture.soap_response("Browse", {"ObjectID": "42"}), timeout=0.1)


class WorkspaceTest(unittest.TestCase):
    def test_success_removes_owned_workspace(self):
        with fixture_workspace() as root:
            (root / "file").write_text("test")
        self.assertFalse(root.exists())

    def test_failure_preserves_files_and_prints_log(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture"
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                with self.assertRaisesRegex(ValueError, "injected failure"):
                    with fixture_workspace(path) as root:
                        (root / "server-0001.log").write_text("failure detail")
                        raise ValueError("injected failure")
            self.assertTrue((path / "failure.json").exists())
            self.assertIn("failure detail", output.getvalue())

    def test_explicit_workspace_is_not_overwritten_or_removed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture"
            with fixture_workspace(path):
                pass
            self.assertTrue(path.exists())
            with self.assertRaises(FileExistsError):
                with fixture_workspace(path):
                    self.fail("existing evidence directory was reused")


@unittest.skipUnless(sys.platform == "linux", "Kernel inotify evidence requires Linux")
class InotifyReadinessTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.libc = ctypes.CDLL(None, use_errno=True)
        self.libc.inotify_init1.argtypes = [ctypes.c_int]
        self.libc.inotify_init1.restype = ctypes.c_int
        self.libc.inotify_add_watch.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_uint32]
        self.libc.inotify_add_watch.restype = ctypes.c_int
        self.fd = self.libc.inotify_init1(os.O_NONBLOCK | os.O_CLOEXEC)
        self.assertGreaterEqual(self.fd, 0)
        self.addCleanup(os.close, self.fd)
        self.server = GerberaProcess("unused", "unused", "127.0.0.1", self.root)
        # Observe our test descriptor only; never signal this process.
        self.server.process = Mock(pid=os.getpid())
        self.server.process.poll.return_value = None

    def watch(self, path, mask=0x3C8):
        result = self.libc.inotify_add_watch(self.fd, os.fsencode(path), mask)
        self.assertGreaterEqual(result, 0)

    def test_waits_for_delayed_kernel_registration(self):
        timer = threading.Timer(0.2, self.watch, args=(self.root,))
        started = time.monotonic()
        timer.start()
        try:
            self.server.wait_for_watches((self.root,), timeout=2)
        finally:
            timer.join()
        self.assertGreaterEqual(time.monotonic() - started, 0.2)

    def test_other_directory_does_not_satisfy_watch_requirement(self):
        other = self.root / "other"
        other.mkdir()
        self.watch(other)
        with self.assertRaisesRegex(FixtureError, "not registered"):
            self.server.wait_for_watches((self.root,), timeout=0.1)

    def test_incomplete_event_mask_does_not_satisfy_requirement(self):
        self.watch(self.root, mask=0x100)
        with self.assertRaisesRegex(FixtureError, "not registered"):
            self.server.wait_for_watches((self.root,), timeout=0.1)


@unittest.skipUnless(os.name == "posix", "Integration process fixtures use POSIX")
class LifecycleTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.config = self.root / "config.xml"
        self.config.write_text("<config><server><udn>uuid:test</udn></server></config>")
        launcher = self.root / "fixture-server"
        # Reuse this file's child mode instead of a second mock server program.
        launcher.write_text(
            f'#!/bin/sh\nexec {shlex.quote(sys.executable)} '
            f'{shlex.quote(str(Path(__file__).resolve()))} "$@"\n'
        )
        launcher.chmod(0o700)
        self.server = GerberaProcess(launcher, self.config, "127.0.0.1", self.root)
        self.addCleanup(self.server.stop)

    def configure(self, **values):
        root = ET.parse(self.config).getroot()
        for name, value in values.items():
            ET.SubElement(root, name.replace("_", "-")).text = str(value)
        ET.ElementTree(root).write(self.config)

    def test_bound_port_does_not_release_start_before_ready(self):
        self.configure(delay=0.25)
        started = time.monotonic()
        called = []
        self.server.start(lambda: called.append(time.monotonic()))
        self.assertGreaterEqual(called[0] - started, 0.25)
        self.assertIsNotNone(self.server.host)

    def test_empty_catalog_readiness_return_is_accepted(self):
        self.server.start(lambda: [])
        self.server.check_alive()

    def test_early_exit_fails_without_waiting_for_deadline(self):
        self.configure(mode="exit")
        started = time.monotonic()
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(FixtureError, "status 7"):
                self.server.start(lambda: True, timeout=3)
        self.assertLess(time.monotonic() - started, 2)
        self.assertIsNone(self.server.process)

    def test_missing_ready_times_out_and_reaps_process(self):
        self.configure(mode="never-ready")
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(FixtureError, "converge"):
                self.server.start(lambda: True, timeout=0.2)
        self.assertIsNone(self.server.process)
        self.assertIn("Server bound", self.server.log_path.read_text())

    def test_wrong_instance_fails_before_test_callback(self):
        self.configure(actual_udn="uuid:unrelated")
        check = Mock()
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(FixtureError, "Wrong server identity"):
                self.server.start(check)
        check.assert_not_called()

    def test_restart_cannot_reuse_old_ready_or_endpoint(self):
        self.server.start(lambda: True)
        first_log = self.server.log_path
        self.server.stop()
        self.configure(delay=0.25)
        started = time.monotonic()
        self.server.start(lambda: True)
        self.assertGreaterEqual(time.monotonic() - started, 0.25)
        self.assertNotEqual(first_log, self.server.log_path)
        self.assertTrue(first_log.exists())

    def test_ready_callback_failure_also_reaps_process(self):
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(ValueError, "injected failure"):
                self.server.start(Mock(side_effect=ValueError("injected failure")))
        self.assertIsNone(self.server.process)

    def test_hung_shutdown_is_killed_and_reported(self):
        self.configure(mode="ignore-term")
        self.server.start(lambda: True)
        with self.assertRaisesRegex(FixtureError, "did not stop gracefully"):
            self.server.stop(timeout=0.1)
        self.assertIsNone(self.server.process)

    def test_start_error_is_preserved_when_cleanup_also_fails(self):
        self.configure(mode="ignore-term")
        stop = self.server.stop
        with patch.object(self.server, "stop", side_effect=lambda: stop(timeout=0.1)):
            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaisesRegex(FixtureError, "injected failure") as result:
                    self.server.start(Mock(side_effect=ValueError("injected failure")))
        self.assertIsInstance(result.exception.__cause__, ValueError)
        self.assertIn("Cleanup also failed: Gerbera did not stop gracefully", str(result.exception))
        self.assertIsNone(self.server.process)


if __name__ == "__main__":
    if "-c" in sys.argv:
        child()
    else:
        unittest.main()
