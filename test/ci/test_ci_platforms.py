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

"""Partial selection and exact-tree completion, including hostile API history."""

import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
import urllib.error

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / ".github/scripts"))
import ci_platforms as P
import ci_selection as CI
from select_web_tests import SelectionError, api_pages
from test_ci_selection import results


ROOT = Path(__file__).resolve().parents[2]
EVENT = {"number": 42, "pull_request": {"head": {"sha": "a" * 40},
         "base": {"sha": "b" * 40}, "changed_files": 3, "labels": []}}
REGISTRY = json.loads((ROOT / ".github/ci-platforms.json").read_text())


class PlatformTests(unittest.TestCase):
    def selection(self, patterns=None, completion=False, receipts=None, event_name="pull_request"):
        event = copy.deepcopy(EVENT)
        if completion:
            event["pull_request"]["labels"] = [{"name": P.COMPLETE_LABEL}]
        return P.plan(CI.full("test"), {"macos_first": False, "platforms": patterns or ["*"]},
                      REGISTRY, CI.JOBS, event_name, event, "f" * 64, receipts)

    def test_default_preserves_all_eligible_variants(self):
        selection = self.selection()
        plan = P.validate_plan(selection)
        self.assertEqual([len(plan[key + "_matrix"]["include"]) for key in ("cpp", "head_check", "minimal", "deb_package")], [6, 2, 26, 10])
        self.assertEqual(plan["excluded"], [])
        self.assertEqual(CI.final_errors(results(selection)), [])
        release = self.selection(event_name="schedule")["platform_plan"]
        self.assertEqual(len(release["deb_package_matrix"]["include"]), 30)

    def test_subset_then_completion_runs_only_missing_targets(self):
        initial = self.selection(["cpp/gcc-13", "minimal/x86_64/v3.24", "minimal/aarch64/v3.24"])
        plan = P.validate_plan(initial)
        self.assertEqual(len(plan["cpp_matrix"]["include"]), 1)
        self.assertEqual(len(plan["minimal_matrix"]["include"]), 2)
        self.assertFalse(plan["FreeBSD_run"])
        self.assertTrue(plan["excluded"])
        self.assertEqual(CI.final_errors(results(initial)), [])
        receipts = {target: {"success": True, "run": 10, "job": i + 1}
                    for i, target in enumerate(plan["pending"])}
        completed = self.selection(["cpp/gcc-13"], completion=True, receipts=receipts)
        more = P.validate_plan(completed)
        self.assertFalse(set(plan["pending"]) & set(more["pending"]))
        self.assertEqual(set(more["pending"]), set(plan["excluded"]))
        self.assertEqual(more["excluded"], [])
        self.assertFalse(more["js_run"])  # Web/format/XML successes need no repeat.
        self.assertEqual(CI.final_errors(results(completed)), [])
        broken = results(completed)
        broken["minimal"]["result"] = "failure"
        self.assertTrue(CI.final_errors(broken))

    def test_missing_outputs_or_invalid_receipts_block_completion(self):
        selection = self.selection()
        broken = results(selection)
        del broken["selection"]["outputs"]["minimal_run"]
        self.assertTrue(CI.final_errors(broken))
        for proof in [{"success": False, "run": 1, "job": 2}, {"success": True, "run": 1}]:
            with self.subTest(proof=proof), self.assertRaises(SelectionError):
                P.validate_plan(self.selection(receipts={"cpp/gcc-13": proof}))
        broken = results(selection)
        broken["selection"]["outputs"]["cpp_matrix"] = json.dumps({"include": []})
        self.assertTrue(CI.final_errors(broken))

    def test_mac_first_can_reuse_mac_and_already_complete_runs_allocate_nothing(self):
        first = CI.full("test")
        first["macos_first"] = True
        event = copy.deepcopy(EVENT)
        event["pull_request"]["labels"] = [{"name": P.COMPLETE_LABEL}]
        reused = {"macos": {"success": True, "run": 10, "job": 1}}
        selected = P.plan(first, {"macos_first": True}, REGISTRY, CI.JOBS, "pull_request", event, "f" * 64, reused)
        self.assertFalse(selected["platform_plan"]["macos_first_run"])
        self.assertTrue(selected["platform_plan"]["cpp_run"])
        self.assertEqual(CI.final_errors(results(selected)), [])
        reused = {target: {"success": True, "run": 10, "job": i + 1}
                  for i, target in enumerate(selected["platform_plan"]["required"])}
        done = self.selection(completion=True, receipts=reused)
        self.assertEqual(done["platform_plan"]["pending"], [])
        self.assertTrue(all(not value for key, value in done["platform_plan"].items() if key.endswith("_run")))
        self.assertEqual(CI.final_errors(results(done)), [])

    def test_prepare_rechecks_pr_base_after_collecting_history(self):
        event = copy.deepcopy(EVENT)
        event["pull_request"]["labels"] = [{"name": P.COMPLETE_LABEL}]
        changed = copy.deepcopy(event["pull_request"])
        changed["base"]["sha"] = "e" * 40
        with patch.object(P.subprocess, "check_output", return_value="c" * 40), \
                patch.object(P, "previous_successes", return_value={}):
            with self.assertRaisesRegex(SelectionError, "PR changed"):
                P.prepare(CI.full("test"), {"macos_first": False}, CI.JOBS, "pull_request", event,
                          lambda path: changed, "gerbera/gerbera", 11)

    def test_unknown_empty_or_misspelled_platforms_fail(self):
        for patterns in [[], ["minimal/pcc64/*"], ["cpp/gcc-13", "cpp/gcc-13"], [None], [{}]]:
            with self.subTest(patterns=patterns), self.assertRaises(SelectionError):
                policy = {"macos_first": False, "platforms": patterns}
                P.plan(CI.full("test"), policy, REGISTRY, CI.JOBS, "pull_request", EVENT)

    def test_head_base_tree_and_pr_identity_invalidate_receipts(self):
        original = P.fingerprint(EVENT, "c" * 40)
        for field in ["head", "base"]:
            changed = copy.deepcopy(EVENT)
            changed["pull_request"][field]["sha"] = "d" * 40
            self.assertNotEqual(P.fingerprint(changed, "c" * 40), original)
        self.assertNotEqual(P.fingerprint(EVENT, "d" * 40), original)
        self.assertNotEqual(P.fingerprint({**EVENT, "number": 43}, "c" * 40), original)

    def test_full_schedule_ignores_partial_policy(self):
        value = self.selection(["cpp/gcc-13"], event_name="schedule")
        self.assertEqual(value["platform_plan"]["excluded"], [])


class HistoryTests(unittest.TestCase):
    def job(self, number, outcome="success", attempt=1, identity="f" * 64, marker=True):
        return {"id": number, "run_attempt": attempt,
                "name": "Compile gcc-13",
                "status": "completed", "conclusion": outcome,
                "steps": [{"name": f"Record platform completion [ci:{identity}:cpp/gcc-13]" if marker else "Check selected coverage",
                           "conclusion": "success"}]}

    def history(self, jobs, run_status="completed", run_conclusion="success"):
        run = {"id": 10, "path": ".github/workflows/ci.yml",
               "head_sha": "a" * 40, "event": "pull_request", "status": run_status, "conclusion": run_conclusion}
        def fetch(path):
            if "/jobs?" in path:
                return {"total_count": len(jobs), "jobs": jobs}
            if path.endswith("/runs/10"):
                return run
            return {"total_count": 1, "workflow_runs": [run]}
        return P.previous_successes(fetch, "gerbera/gerbera", "a" * 40, "f" * 64, 11)

    def test_only_actual_payload_success_is_reused(self):
        self.assertIn("cpp/gcc-13", self.history([self.job(1)]))
        for job in [self.job(1, "failure"), self.job(1, "cancelled"), self.job(1, "skipped"),
                    self.job(1, identity="e" * 64), self.job(1, marker=False)]:
            with self.subTest(job=job):
                self.assertEqual(self.history([job]), {})

    def test_latest_attempt_failure_supersedes_success_and_retry_can_recover(self):
        self.assertEqual(self.history([self.job(1), self.job(2, "failure", 2)]), {})
        proof = self.history([self.job(1, "failure"), self.job(2, attempt=2)])
        self.assertEqual(proof["cpp/gcc-13"]["job"], 2)

    def test_active_empty_or_unreadable_history_is_not_permission_to_rerun(self):
        for jobs, status in [([self.job(1)], "queued"), ([], "completed")]:
            with self.subTest(status=status), self.assertRaises(SelectionError):
                self.history(jobs, status)
        def unavailable(path):
            raise urllib.error.HTTPError(path, 503, "unavailable", {}, None)
        with self.assertRaises(urllib.error.HTTPError):
            P.previous_successes(unavailable, "gerbera/gerbera", "a" * 40, "f" * 64, 11)

    def test_rerun_started_during_job_collection_blocks_reuse(self):
        run = {"id": 10, "path": ".github/workflows/ci.yml", "head_sha": "a" * 40,
               "event": "pull_request", "status": "completed", "run_attempt": 1}
        def fetch(path):
            if "/jobs?" in path:
                return {"total_count": 1, "jobs": [self.job(1)]}
            if path.endswith("/runs/10"):
                return {**run, "status": "queued", "run_attempt": 2}
            return {"total_count": 1, "workflow_runs": [run]}
        with self.assertRaisesRegex(SelectionError, "workflow changed"):
            P.previous_successes(fetch, "gerbera/gerbera", "a" * 40, "f" * 64, 11)

    def test_cancelled_before_execution_provides_no_receipt(self):
        self.assertEqual(self.history([], run_conclusion="cancelled"), {})
        job = self.job(1, "cancelled")
        job["steps"] = []
        self.assertEqual(self.history([job], run_conclusion="cancelled"), {})

    def test_all_pages_are_read_and_incomplete_totals_fail(self):
        calls = []
        def fetch(path):
            calls.append(path)
            page = int(path.rsplit("=", 1)[1])
            return {"total_count": 201, "jobs": [{"id": i} for i in range((page - 1) * 100, min(page * 100, 201))]}
        self.assertEqual(len(api_pages(fetch, "jobs?filter=all", "jobs")), 201)
        self.assertEqual(len(calls), 3)
        with self.assertRaises(SelectionError):
            api_pages(lambda path: {"total_count": 101, "jobs": []}, "jobs", "jobs")


if __name__ == "__main__":
    unittest.main()
