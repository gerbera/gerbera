"""Group dependencies, optional scheduling, formatting batches and merge gate."""

import contextlib
import copy
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import urllib.error

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / ".github/scripts"))
import ci_selection as CI
from select_web_tests import SelectionError
from test_select_web_tests import FakeAPI, EVENT, change


def record(path, **extra):
    return {"filename": path, "status": "modified", **extra}


def results(selection):
    outputs = {key: str(value).lower() for key, value in selection["groups"].items()}
    outputs.update(selection=json.dumps(selection), macos_first=str(selection["macos_first"]).lower())
    needs = {"selection": {"result": "success", "outputs": outputs}}
    for job in CI.JOBS:
        status = "success"
        if job in ("macos", "macos-first") and (
                not selection["groups"]["macos"] or selection["macos_first"] != (job == "macos-first")):
            status = "skipped"
        needs[job] = {"result": status}
    return needs


class GroupTests(unittest.TestCase):
    def selected(self, *paths):
        selection = CI.classify_groups([record(path) for path in paths])
        CI.validate_selection(selection)
        return {key for key, selected in selection["groups"].items() if selected}

    def test_independent_groups(self):
        cases = {
            "doc/config-import.rst": {"security"},
            ".github/workflows/minimal.yml": {"minimal", "security"},
            "gerbera-web/package-lock.json": {"web", "security"},
            "web/js/gerbera.items.js": {"web", "deb", "docker", "security"},
            "scripts/sunos/build-solaris.sh": {"omnios", "security"},
            "scripts/debian/postinst": {"deb", "head", "security"},
            "scripts/docker/docker-entrypoint.sh": {"docker", "security"},
            "scripts/alpine/deps/taglib.sh": {"minimal", "docker", "security"},
            "Dockerfile": {"docker", "security"},
            ".clang-format": {"format", "format_cpp", "security"},
            ".cmake-format.py": {"format", "format_cmake", "security"},
            "doc/gerbera.1": {"deb", "head", "security"},
        }
        for path, expected in cases.items():
            with self.subTest(path=path):
                self.assertEqual(self.selected(path), expected)

    def test_native_configuration_import_and_database_consumers(self):
        for path in ["src/config/config_definition.cc", "src/database/sqlite3/sqlite3.sql",
                     "src/content/scripting/parser_script.h", "scripts/js/common.js",
                     "test/config/test_configmanager.cc", "test/database/test_database.cc",
                     "test/scripting/test_runtime.cc", "config/config2.xsd"]:
            with self.subTest(path=path):
                selected = self.selected(path)
                self.assertTrue(set(CI.NATIVE).issubset(selected))
                self.assertIn("macos", selected)
                if path.startswith(("config/", "src/config/", "test/config/")):
                    self.assertIn("xml", selected)

    def test_shared_build_dependencies_and_unknown_inputs_keep_mac_and_full_coverage(self):
        for path in ["CMakeLists.txt", "cmake/FindTaglib.cmake", "CMakePresets.json",
                     "conanfile.py", "conan/dev", "scripts/install-taglib.sh",
                     ".github/workflows/ci.yml", ".github/ci-policy.json",
                     ".github/scripts/ci_selection.py", "test/ci/new.py", "unknown/new.file"]:
            with self.subTest(path=path):
                self.assertEqual(self.selected(path), set(CI.GROUPS) | {"format"})

    def test_mixed_paths_renames_deletions_and_security(self):
        selected = self.selected("doc/ui.rst", "web/index.html", "scripts/sunos/build-solaris.sh")
        self.assertEqual(selected, {"web", "deb", "docker", "omnios", "security"})
        renamed = CI.classify_groups([record("doc/old.md", status="renamed", previous_filename="src/config/old.cc")])
        self.assertTrue(renamed["groups"]["macos"])
        self.assertTrue(renamed["groups"]["xml"])
        removed = CI.classify_groups([record("web/deleted.js", status="removed")])
        self.assertTrue(removed["groups"]["docker"])
        for flags in [CI.full("fallback"), CI.classify_groups([record("README.md")])]:
            self.assertTrue(flags["groups"]["security"])


class PolicyTests(unittest.TestCase):
    def test_parallel_default_and_repository_opt_in(self):
        self.assertFalse(CI.macos_first({"macos_first": False}, "pull_request", {}))
        self.assertTrue(CI.macos_first({"macos_first": True}, "pull_request", {}))
        for event in ["push", "schedule"]:
            self.assertTrue(CI.macos_first({"macos_first": True}, event, {"inputs": {"execution-order": "parallel"}}))

    def test_manual_override_in_both_directions(self):
        for default in [True, False]:
            for order, expected in [("parallel", False), ("macos-first", True), ("repository-default", default)]:
                with self.subTest(default=default, order=order):
                    self.assertIs(CI.macos_first({"macos_first": default}, "workflow_dispatch", {"inputs": {"execution-order": order}}), expected)

    def test_invalid_policy_does_not_silently_change_order(self):
        for policy in [{}, {"macos_first": "false"}, {"macos_first": 0}, {"macos_first": False, "typo": True}]:
            with self.subTest(policy=policy), self.assertRaises(SelectionError):
                CI.macos_first(policy, "pull_request", {})
        with self.assertRaises(SelectionError):
            CI.macos_first({"macos_first": False}, "workflow_dispatch", {"inputs": {"execution-order": "typo"}})


class CompletionTests(unittest.TestCase):
    def test_parallel_and_mac_first_have_exactly_one_mac_execution(self):
        for first in [True, False]:
            selection = CI.full("test")
            selection["macos_first"] = first
            needs = results(selection)
            self.assertEqual(CI.final_errors(needs), [])
            unused = "macos" if first else "macos-first"
            needs[unused]["result"] = "success"
            self.assertTrue(CI.final_errors(needs))
        self.assertEqual(CI.final_errors(results(CI.classify_groups([record("README.md")]))), [])

    def test_every_selected_failure_cancel_skip_or_missing_result_blocks(self):
        for first in [True, False]:
            selection = CI.full("test")
            selection["macos_first"] = first
            for job in CI.JOBS:
                if job == ("macos" if first else "macos-first"):
                    continue
                for outcome in ["failure", "cancelled", "skipped", "", None]:
                    needs = results(selection)
                    needs[job]["result"] = outcome
                    with self.subTest(first=first, job=job, outcome=outcome):
                        self.assertTrue(CI.final_errors(needs))
            needs = results(selection)
            del needs["cpp"]
            self.assertTrue(CI.final_errors(needs))

    def test_selection_failure_and_output_inconsistency_block(self):
        for outcome in ["failure", "cancelled", "skipped", None]:
            needs = results(CI.full("test"))
            needs["selection"]["result"] = outcome
            self.assertTrue(CI.final_errors(needs))
        for field, value in [("native", "false"), ("macos_first", "true"), ("selection", "invalid JSON")]:
            needs = results(CI.full("test"))
            needs["selection"]["outputs"][field] = value
            self.assertTrue(CI.final_errors(needs))

    def test_actual_completion_cli_exit_status(self):
        needs = results(CI.full("test"))
        for failed in [False, True]:
            if failed:
                needs["format"]["result"] = "failure"
            result = subprocess.run([sys.executable, "-B", str(ROOT / ".github/scripts/ci_selection.py"), "--final-result"],
                                    env={**os.environ, "NEEDS_JSON": json.dumps(needs)}, capture_output=True, text=True)
            self.assertEqual(result.returncode, int(failed), result.stdout + result.stderr)


class MainTests(unittest.TestCase):
    def execute(self, api, event_name="pull_request", order="repository-default"):
        with tempfile.TemporaryDirectory() as directory, contextlib.chdir(directory):
            Path(".github").mkdir()
            Path(".github/ci-policy.json").write_text(json.dumps({"macos_first": False}))
            event = {**EVENT, "inputs": {"execution-order": order}}
            Path("event.json").write_text(json.dumps(event))
            env = {**os.environ, "GITHUB_EVENT_NAME": event_name,
                   "GITHUB_EVENT_PATH": "event.json", "GITHUB_REPOSITORY": "gerbera/gerbera",
                   "GITHUB_OUTPUT": "output", "GITHUB_STEP_SUMMARY": "summary"}
            with patch.dict(os.environ, env), patch.object(sys, "argv", ["ci_selection.py"]), \
                    patch.object(CI, "api_fetch", return_value=api), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(CI.main(), 0)
            outputs = dict(line.split("=", 1) for line in Path("output").read_text().splitlines())
            return json.loads(outputs["selection"]), outputs, Path("summary").read_text()

    def test_complete_pr_snapshot_drives_group_outputs(self):
        for paths, native in [([f"doc/page{i}.rst" for i in range(201)], False),
                              (["src/config/config_definition.cc"], True)]:
            with self.subTest(native=native):
                api = FakeAPI([change(path) for path in paths])
                selection, outputs, summary = self.execute(api)
                self.assertIs(selection["groups"]["native"], native)
                self.assertEqual(outputs["native"], str(native).lower())
                self.assertEqual(api.metadata_reads, 2)
                self.assertIn("selected" if native else "not applicable / not executed", summary)

    def test_api_failure_or_changed_snapshot_uses_full_groups(self):
        def unavailable(path):
            raise urllib.error.HTTPError(path, 503, "unavailable", {}, None)
        changed = FakeAPI([change("doc/guide.rst")])
        changed.final_head = "c" * 40
        for api in [unavailable, changed]:
            selection, _, _ = self.execute(api)
            self.assertTrue(all(selection["groups"].values()))
            self.assertIn("full fallback", selection["reason"])

    def test_manual_full_run_and_order_override_do_not_query_pr(self):
        def forbidden(path):
            self.fail("Manual full run queried the PR API")
        for order, first in [("parallel", False), ("macos-first", True)]:
            selection, outputs, _ = self.execute(forbidden, "workflow_dispatch", order)
            self.assertTrue(all(selection["groups"].values()))
            self.assertIs(selection["macos_first"], first)
            self.assertEqual(outputs["macos_first"], str(first).lower())


class FormatterTests(unittest.TestCase):
    def test_collected_files_keep_argument_boundaries_and_failures(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.chdir(directory):
            Path("cmake").mkdir()
            files = ["CMakeLists.txt", "cmake/has spaces.cmake", "cmake/deleted.cmake"]
            for path in files[:2]:
                Path(path).write_text("project(test)\n")
            selection = CI.full("test")
            selection.update(all_cmake=False, cmake_files=files)
            calls = []
            def execute(args, check):
                calls.append(args)
                return SimpleNamespace(returncode=int("spaces" in args[-1]))
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(CI.format_cmake(selection, execute), 1)
            self.assertEqual([args[-1] for args in calls], ["./CMakeLists.txt", "./cmake/has spaces.cmake"])
            self.assertTrue(all(args[:4] == ["cmake-format", "-c", "./.cmake-format.py", "--check"] for args in calls))

    def test_full_format_manifest_keeps_all_original_matrix_inputs(self):
        paths = json.loads((ROOT / ".github/cmake-format-files.json").read_text())
        self.assertEqual(len(paths), 40)
        self.assertEqual(len(set(paths)), 40)
        self.assertTrue(all((ROOT / path).is_file() for path in paths))


if __name__ == "__main__":
    unittest.main()
