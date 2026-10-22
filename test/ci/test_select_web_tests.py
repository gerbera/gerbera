"""Coverage selection and required-check safety, without network or npm."""

import contextlib
import importlib.util
import io
import json
import os
import subprocess
import sys
import tempfile
import unittest
import urllib.error
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / ".github/scripts/select_web_tests.py"
SPEC = importlib.util.spec_from_file_location("select_web_tests", SCRIPT)
SELECTOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SELECTOR)
HEAD, BASE = "a" * 40, "b" * 40
EVENT = {"number": 7, "pull_request": {"head": {"sha": HEAD}, "base": {"sha": BASE}}}


def change(path, status="modified", **extra):
    return {"filename": path, "status": status, **extra}


class FakeAPI:
    def __init__(self, files):
        self.files = files
        self.calls = []
        self.metadata_reads = 0
        self.count = len(files)
        self.final_head = HEAD

    def __call__(self, path):
        self.calls.append(path)
        if "/files?" not in path:
            self.metadata_reads += 1
            return {"head": {"sha": HEAD if self.metadata_reads == 1 else self.final_head},
                    "base": {"sha": BASE}, "changed_files": self.count}
        page = int(path.rsplit("=", 1)[1])
        return self.files[(page - 1) * 100:page * 100]


class ClassificationTests(unittest.TestCase):
    def test_component_map(self):
        cases = [
            (["doc/config-import.rst"], False),
            (["README.md", "doc/guide.md"], False),
            ([".github/workflows/minimal.yml"], False),
            ([".github/workflows/minimal.yml", "doc/compile.rst"], False),
            (["gerbera-web/package-lock.json"], True),
            (["gerbera-web/mock-api/mock.responder.js"], True),
            (["web/js/gerbera-items.js"], True),
            (["src/web/web_request_handler.cc"], True),
            (["src/content/content.cc", "doc/layout.rst"], True),
            (["CMakeLists.txt"], True),
            (["cmake/FindUPNP.cmake"], True),
            (["scripts/js/common.js"], True),
            (["config/config.xml"], True),
            (["doc/CMakeLists.txt"], True),
            (["doc/conf.py"], True),
            (["doc/_static/gerbera-logo.png"], True),
            ([".github/scripts/select_web_tests.py"], True),
            (["test/ci/test_select_web_tests.py"], True),
            ([".github/workflows/ci-js.yml"], True),
            (["unknown/file.xyz"], True),
            (["doc/guide.rst", "web/index.html"], True),
        ]
        for paths, expected in cases:
            with self.subTest(paths=paths):
                self.assertIs(SELECTOR.classify([change(path) for path in paths])[0], expected)

    def test_renames_and_deletions(self):
        for old, new, selected in [("src/a.cc", "doc/a.md", True),
                                   ("doc/a.md", "web/a.js", True),
                                   ("doc/a.md", "doc/b.md", False)]:
            with self.subTest(old=old, new=new):
                self.assertIs(SELECTOR.classify([change(new, "renamed", previous_filename=old)])[0], selected)
        self.assertTrue(SELECTOR.classify([change("web/a.js", "removed")])[0])
        self.assertFalse(SELECTOR.classify([change("doc/a.rst", "removed")])[0])

    def test_rejects_incomplete_or_ambiguous_records(self):
        for files in [[], None, [{}], [None], [change("doc/a.rst", "renamed")],
                      [change("doc/a.rst", "copied")],
                      [change("doc/a.rst"), change("doc/a.rst")]]:
            with self.subTest(files=files), self.assertRaises(SELECTOR.SelectionError):
                SELECTOR.classify(files)
        for path in [None, "", "/doc/a.md", "doc//a.md", "doc/../a.md",
                     "doc/./a.md", "doc\\a.md", "doc/a\n.md"]:
            with self.subTest(path=path), self.assertRaises(SELECTOR.SelectionError):
                SELECTOR.classify([change(path)])
        self.assertFalse(SELECTOR.classify([change("doc/日本語.rst")])[0])


class ChangeSetTests(unittest.TestCase):
    def select(self, api, event=EVENT, event_name="pull_request"):
        return SELECTOR.select(event_name, event, "gerbera/gerbera", api)

    def test_pagination_and_snapshot(self):
        api = FakeAPI([change(f"doc/page{i}.rst") for i in range(201)])
        self.assertFalse(self.select(api)[0])
        self.assertEqual(api.metadata_reads, 2)
        self.assertEqual(len([call for call in api.calls if "/files?" in call]), 3)

    def test_exact_hundred_boundary_is_not_truncated(self):
        api = FakeAPI([change(f"doc/page{i}.rst") for i in range(100)])
        self.assertFalse(self.select(api)[0])
        self.assertIn("repos/gerbera/gerbera/pulls/7/files?per_page=100&page=2", api.calls)

    def test_races_and_limits_are_not_exempted(self):
        api = FakeAPI([change("doc/a.rst")])
        api.final_head = "c" * 40
        with self.assertRaises(SELECTOR.SelectionError):
            self.select(api)
        for count in [0, -1, 2, 3000, 4000, True, "1"]:
            api = FakeAPI([change("doc/a.rst")])
            api.count = count
            with self.subTest(count=count), self.assertRaises(SELECTOR.SelectionError):
                self.select(api)

    def test_event_head_and_base_must_match(self):
        for field in ["head", "base"]:
            event = json.loads(json.dumps(EVENT))
            event["pull_request"][field]["sha"] = "c" * 40
            with self.subTest(field=field), self.assertRaises(SELECTOR.SelectionError):
                self.select(FakeAPI([change("doc/a.rst")]), event)

    def test_malformed_metadata(self):
        for metadata in [{}, [], {"head": {"sha": HEAD}, "base": {"sha": BASE}, "changed_files": None}]:
            with self.subTest(metadata=metadata), self.assertRaises(SELECTOR.SelectionError):
                self.select(lambda _: metadata)

    def test_non_pr_events_need_no_api(self):
        for event_name in ["push", "workflow_dispatch", "schedule", "release", "unknown"]:
            with self.subTest(event_name=event_name):
                self.assertTrue(self.select(lambda _: self.fail("unexpected API request"), {}, event_name)[0])

    def test_drafts_use_same_rules(self):
        event = {**EVENT, "pull_request": {**EVENT["pull_request"], "draft": True}}
        self.assertFalse(self.select(FakeAPI([change("doc/a.rst")]), event)[0])


class ExecutionTests(unittest.TestCase):
    def environment(self, selected="true"):
        return {"RUN_WEB": selected, "SELECTOR_TESTS_OUTCOME": "success",
                "SELECTION_OUTCOME": "success", "NODE_OUTCOME": "success" if selected == "true" else "skipped"}

    def test_gate_checks_outcomes_not_just_selection(self):
        for selected in ["true", "false"]:
            self.assertIsNone(SELECTOR.execution_error(self.environment(selected)))
            for key in ["SELECTOR_TESTS_OUTCOME", "SELECTION_OUTCOME", "NODE_OUTCOME"]:
                expected = self.environment(selected)[key]
                for outcome in ["", "failure", "cancelled", "skipped", "success"]:
                    if outcome == expected:
                        continue
                    env = {**self.environment(selected), key: outcome}
                    with self.subTest(selected=selected, key=key, outcome=outcome):
                        self.assertIsNotNone(SELECTOR.execution_error(env))
        for value in ["", "True", "FALSE", "garbage", None]:
            self.assertIsNotNone(SELECTOR.execution_error(self.environment(value)))

    def test_actual_cli_gate_exit_status(self):
        for selected, node, expected in [("false", "skipped", 0), ("true", "success", 0),
                                         ("true", "skipped", 1), ("false", "success", 1)]:
            env = {**os.environ, **self.environment(selected), "NODE_OUTCOME": node}
            result = subprocess.run([sys.executable, "-B", str(SCRIPT), "--check-result"],
                                    env=env, capture_output=True, text=True)
            with self.subTest(selected=selected, node=node):
                self.assertEqual(result.returncode, expected, result.stdout + result.stderr)

    def test_api_and_parse_errors_fall_back_to_full_web(self):
        failures = [urllib.error.HTTPError("https://api.github.com/", 403, "Forbidden", {}, None),
                    urllib.error.URLError("timeout"), TimeoutError(),
                    json.JSONDecodeError("bad response", "", 0),
                    SELECTOR.SelectionError("incomplete file list")]
        for failure in failures:
            with self.subTest(failure=type(failure).__name__):
                self.run_main(failure=failure, expected=True)

    def test_main_outputs_and_summary(self):
        self.run_main(expected=False)

    def run_main(self, failure=None, expected=False):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            event = directory / "event.json"
            event.write_text(json.dumps(EVENT), encoding="utf-8")
            output, summary = directory / "output", directory / "summary"
            env = {"GITHUB_EVENT_PATH": str(event), "GITHUB_EVENT_NAME": "pull_request",
                   "GITHUB_REPOSITORY": "gerbera/gerbera", "GITHUB_OUTPUT": str(output),
                   "GITHUB_STEP_SUMMARY": str(summary)}
            fetch = FakeAPI([change("doc/a.rst")])
            if failure:
                def fetch(_):
                    raise failure
            with patch.dict(os.environ, env), patch.object(sys, "argv", [str(SCRIPT)]), \
                    patch.object(SELECTOR, "api_fetch", return_value=fetch), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(SELECTOR.main(), 0)
            self.assertEqual(output.read_text(), f"web={str(expected).lower()}\n")
            self.assertIn("selected" if expected else "not applicable (not executed)", summary.read_text())


if __name__ == "__main__":
    unittest.main()
