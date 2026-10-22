#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise mirror selection and real curl/TLS retry on isolated fixtures."""
import http.server
import io
import json
import os
from pathlib import Path
import ssl
import subprocess
import tarfile
import tempfile
import threading
import unittest

SCRIPT = Path(__file__).with_name("select-alpine-mirror.sh")


def index_bytes(text=b"P:busybox\nV:1\n"):
    output = io.BytesIO()
    with tarfile.open(fileobj=output, mode="w:gz") as archive:
        item = tarfile.TarInfo("APKINDEX")
        item.size = len(text)
        archive.addfile(item, io.BytesIO(text))
    return output.getvalue()


class MirrorTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="alpine-test-")
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.log = self.root / "curl.jsonl"
        self.payload = self.root / "index.tar.gz"
        self.payload.write_bytes(index_bytes())
        stub = self.bin / "curl"
        stub.write_text('''#!/usr/bin/env python3
import os,json,sys,pathlib
args=sys.argv[1:];url=args[-1]
with open(os.environ['CURL_LOG'],'a') as f:f.write(json.dumps(args)+'\\n')
mode=os.environ['CURL_MODE'];bad='primary.test' in url
if mode=='all-fail' or (bad and mode=='primary-fail'):sys.exit(22)
path=pathlib.Path(args[args.index('--output')+1])
if bad and mode=='bad-archive':path.write_bytes(b'not gzip')
elif bad and mode=='empty-index':path.write_bytes(pathlib.Path(os.environ['EMPTY_INDEX']).read_bytes())
elif bad and mode=='partial' and '/community/' in url:path.write_bytes(b'partial')
else:path.write_bytes(pathlib.Path(os.environ['INDEX']).read_bytes())
''', encoding="utf-8")
        stub.chmod(0o755)
        (self.root / "empty.tar.gz").write_bytes(index_bytes(b""))

    def tearDown(self):
        self.temp.cleanup()

    def run_selector(self, mode="success", branch="edge", arch="x86", real=False, urls=None, extra=None):
        env = {**os.environ, "RUNNER_TEMP": str(self.root), "CURL_LOG": str(self.log),
               "CURL_MODE": mode, "INDEX": str(self.payload), "EMPTY_INDEX": str(self.root / "empty.tar.gz")}
        if not real:
            env["PATH"] = str(self.bin) + os.pathsep + env["PATH"]
        env.update(extra or {})
        return subprocess.run(["bash", str(SCRIPT), branch, arch,
                               *(urls or ["https://primary.test/alpine", "https://fallback.test/alpine"])],
                              env=env, capture_output=True, text=True, timeout=45)

    def test_primary_and_matching_fallback_repositories(self):
        r = self.run_selector(branch="v3.24", arch="aarch64")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("mirror-url=https://primary.test/alpine", r.stdout)
        self.assertIn("https://fallback.test/alpine/v3.24/main", r.stdout)
        calls = [json.loads(x) for x in self.log.read_text().splitlines()]
        self.assertEqual(len(calls), 2)
        self.assertTrue(all(x[-1].endswith("/aarch64/APKINDEX.tar.gz") for x in calls))
        for flag in ["--ipv4", "--fail", "--retry", "--retry-max-time", "--proto-redir"]:
            self.assertIn(flag, calls[0])
        self.assertNotIn("--insecure", calls[0])

    def test_failed_primary_selects_fallback(self):
        r = self.run_selector("primary-fail")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("mirror-url=https://fallback.test/alpine", r.stdout)
        self.assertIn("extra-repositories=\n", r.stdout)
        self.assertNotIn("https://primary.test", r.stdout)

    def test_corrupt_empty_and_partial_indexes_select_fallback(self):
        for mode in ["bad-archive", "empty-index", "partial"]:
            with self.subTest(mode=mode):
                r = self.run_selector(mode)
                self.assertEqual(r.returncode, 0, r.stderr)
                self.assertIn("mirror-url=https://fallback.test/alpine", r.stdout)

    def test_all_failed_has_no_success_output(self):
        r = self.run_selector("all-fail")
        self.assertNotEqual(r.returncode, 0)
        self.assertEqual(r.stdout, "")
        self.assertIn("::error::", r.stderr)
        self.assertFalse(list(self.root.glob("alpine-mirror.*")))

    def test_matrix_values_and_rejection(self):
        for arch in ["x86_64", "x86", "armhf", "armv7", "aarch64", "loongarch64", "ppc64le", "riscv64", "s390x"]:
            self.assertEqual(self.run_selector(arch=arch).returncode, 0)
        for branch, arch in [("../edge", "x86"), ("edge", "../x86"), ("edge;false", "x86")]:
            self.assertEqual(self.run_selector(branch=branch, arch=arch).returncode, 2)
        self.assertEqual(self.run_selector(urls=["http://primary.test/alpine", "https://fallback.test/alpine"]).returncode, 2)

    def test_real_curl_retries_503_and_preserves_tls_verification(self):
        cert, key = self.root / "cert.pem", self.root / "key.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
                        "-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost",
                        "-keyout", str(key), "-out", str(cert)], check=True, capture_output=True)
        requests = []
        payload = index_bytes()

        class Handler(http.server.BaseHTTPRequestHandler):
            def do_GET(self):
                requests.append(self.path)
                status = 503 if len(requests) <= 2 else 200
                body = b"temporary failure" if status == 503 else payload
                self.send_response(status)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def log_message(self, *args):
                pass

        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert, key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"https://localhost:{server.server_port}/alpine"
        try:
            r = self.run_selector(real=True, urls=[url, "https://localhost:1/alpine"], extra={"CURL_CA_BUNDLE": str(cert)})
            self.assertEqual(r.returncode, 0, r.stderr)
            self.assertIn("mirror-url=" + url, r.stdout)
            self.assertEqual(requests, ["/alpine/edge/main/x86/APKINDEX.tar.gz"] * 3 + ["/alpine/edge/community/x86/APKINDEX.tar.gz"])
            # With the normal CA store this private fixture certificate must fail.
            r = self.run_selector(real=True, urls=[url, url], extra={"CURL_CA_BUNDLE": "/etc/ssl/certs/ca-certificates.crt"})
            self.assertNotEqual(r.returncode, 0)
            self.assertEqual(r.stdout, "")
        finally:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    unittest.main(verbosity=2)
