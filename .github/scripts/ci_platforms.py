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

"""Plan platform jobs and reuse actual successful jobs for one exact PR tree."""

import fnmatch
import hashlib
import json
from pathlib import Path
import re
import subprocess

from select_web_tests import SelectionError, api_pages, pr_snapshot


MATRICES = {"cpp", "head-check", "minimal", "deb-package"}
PLATFORMS = MATRICES | {"macos", "FreeBSD", "OmniOS", "docker-build", "docker-debug"}
ACTIVE = {"queued", "in_progress", "waiting", "pending", "requested"}
COMPLETE_LABEL = "ci:complete-platforms"


def completion_requested(event):
    return any(label.get("name") == COMPLETE_LABEL
               for label in event.get("pull_request", {}).get("labels", []))


def registry_rows(registry, jobs, is_pr):
    if not isinstance(registry, dict) or set(registry) != MATRICES:
        raise SelectionError("missing or unknown platform matrix")
    result = {}
    for job in jobs:
        if job == "macos-first":
            continue  # One logical platform, two alternative scheduling paths.
        rows = registry.get(job, [{"target": job, "label": job}])
        if not isinstance(rows, list) or not rows:
            raise SelectionError("empty platform registry: " + job)
        for row in rows:
            if not isinstance(row, dict) or not isinstance(row.get("label"), str):
                raise SelectionError("invalid platform entry: " + job)
            target = row.get("target")
            if (not isinstance(target, str) or not re.fullmatch(r"[A-Za-z0-9_./:-]+", target)
                    or target in result or target.split("/")[0] != job):
                raise SelectionError("invalid or duplicate platform identifier")
            if job == "deb-package" and is_pr and row.get("pr") is False:
                continue  # Preserve the existing PR-only amd64 packaging boundary.
            result[target] = {**row, "job": job, "group": jobs[job]}
    return result


def validate_policy(policy):
    if (not isinstance(policy, dict) or not {"macos_first"} <= set(policy)
            or set(policy) - {"macos_first", "platforms"}
            or type(policy["macos_first"]) is not bool):
        raise SelectionError("invalid repository CI policy")
    patterns = policy.get("platforms", ["*"])
    if (not isinstance(patterns, list) or not patterns
            or any(not isinstance(value, str) or not re.fullmatch(r"[A-Za-z0-9_./:*?-]+", value)
                   for value in patterns) or len(set(patterns)) != len(patterns)):
        raise SelectionError("platforms must be a nonempty list of unique target patterns")
    return patterns


def fingerprint(event, tree):
    """Bind receipts to PR identity, head, base and the entire tested merge tree.

    The Git tree includes workflow, policy, registry, dependency and test sources.
    Moving the base or changing recipes invalidates every earlier receipt.
    """
    head, base, _ = pr_snapshot(event["pull_request"])
    if not re.fullmatch(r"[0-9a-f]{40}", tree) or type(event.get("number")) is not int:
        raise SelectionError("invalid tested tree or PR identity")
    value = [event["number"], head, base, tree, "platform-receipt-v1"]
    return hashlib.sha256(json.dumps(value, separators=(",", ":")).encode()).hexdigest()


def previous_successes(fetch, repository, head, identity, current_run):
    """Read every page/attempt; a newer failure supersedes an older success.

    A success requires the fingerprinted completion step emitted after the
    actual test/build payload. Existing job/check names remain unchanged.
    """
    runs = api_pages(fetch, f"repos/{repository}/actions/workflows/ci.yml/runs?head_sha={head}&event=pull_request", "workflow_runs", max_pages=10)
    latest = {}
    token = re.compile(r" \[ci:" + re.escape(identity) + r":([A-Za-z0-9_./:-]+)\]$")
    for run in sorted(runs, key=lambda row: row["id"]):
        if run["id"] == current_run:
            continue
        if run.get("head_sha") != head or run.get("event") != "pull_request" or run.get("path") != ".github/workflows/ci.yml":
            raise SelectionError("inconsistent historical workflow metadata")
        if run.get("status") not in ACTIVE | {"completed"}:
            raise SelectionError("unknown historical workflow status")
        if run["status"] != "completed":
            raise SelectionError("earlier PR workflow is still active; wait before completion")
        jobs = api_pages(fetch, f"repos/{repository}/actions/runs/{run['id']}/jobs?filter=all", "jobs")
        # Concurrency can replace a pending run before any job starts. It gives
        # no receipt, but still gets the same final run/attempt consistency check.
        if not jobs and run.get("conclusion") != "cancelled":
            raise SelectionError("historical workflow has no readable jobs")
        for job in sorted(jobs, key=lambda row: (row.get("run_attempt", 1), row["id"])):
            if job.get("status") not in ACTIVE | {"completed"}:
                raise SelectionError("unknown historical job status")
            if job["status"] != "completed":
                raise SelectionError("historical workflow still contains an active job")
            if job.get("conclusion") == "skipped":
                continue  # An unused Mac path cannot overwrite its actual sibling.
            steps = job.get("steps")
            if steps == [] and job.get("conclusion") == "cancelled":
                continue  # No payload ran; this provides no successful receipt.
            if not isinstance(steps, list) or not steps:
                raise SelectionError("missing historical job steps")
            markers = [(step, token.search(step.get("name", ""))) for step in steps
                       if step.get("name", "").startswith("Record platform completion ")]
            markers = [(step, match) for step, match in markers if match]
            if not markers:
                continue
            if len(markers) != 1:
                raise SelectionError("ambiguous platform completion steps")
            marker, match = markers[0]
            success = job.get("conclusion") == "success" and marker.get("conclusion") == "success"
            latest[match[1]] = {"success": success, "run": run["id"], "job": job["id"]}
        after = fetch(f"repos/{repository}/actions/runs/{run['id']}")
        if any(after.get(key) != run.get(key) for key in ("id", "head_sha", "status", "run_attempt")):
            raise SelectionError("workflow changed while collecting platform receipts")
    return {target: proof for target, proof in latest.items() if proof["success"]}


def plan(selection, policy, registry, jobs, event_name, event, identity="", receipts=None):
    patterns = validate_policy(policy)
    rows = registry_rows(registry, jobs, event_name == "pull_request")
    for pattern in patterns:
        if not any(row["job"] in PLATFORMS and fnmatch.fnmatchcase(target, pattern)
                   for target, row in rows.items()):
            raise SelectionError("platform pattern matches no eligible target: " + pattern)
    completion = event_name == "pull_request" and completion_requested(event)
    required = {target for target, row in rows.items() if selection["groups"][row["group"]]}
    chosen = {target for target in required if completion or event_name != "pull_request"
              or rows[target]["job"] not in PLATFORMS
              or any(fnmatch.fnmatchcase(target, pattern) for pattern in patterns)}
    cached = {target: proof for target, proof in (receipts or {}).items() if target in chosen}
    pending = sorted(chosen - cached.keys())
    selection["platform_plan"] = {"fingerprint": identity, "complete": completion,
                                  "required": sorted(required), "excluded": sorted(required - chosen),
                                  "pending": pending, "reused": cached}
    for job in jobs:
        targets = [rows[target] for target in pending if rows[target]["job"] == job]
        key = job.replace("-", "_")
        selection["platform_plan"][key + "_run"] = bool(targets)
        if job in MATRICES:
            selection["platform_plan"][key + "_matrix"] = {"include": [
                {key: value for key, value in row.items() if key not in ("job", "group", "pr")}
                for row in targets]}
    selection["platform_plan"]["macos_first_run"] = bool(
        selection["platform_plan"]["macos_run"] and selection["macos_first"])
    return selection


def validate_plan(selection):
    value = selection["platform_plan"]
    required, excluded, pending, reused = (set(value[key]) for key in ("required", "excluded", "pending", "reused"))
    if (required != excluded | pending | reused or excluded & pending or excluded & reused
            or pending & reused or (value["complete"] and excluded)):
        raise SelectionError("inconsistent incremental platform coverage")
    for proof in value["reused"].values():
        if (proof.get("success") is not True or type(proof.get("run")) is not int
                or type(proof.get("job")) is not int):
            raise SelectionError("reused platform has no successful workflow/job receipt")
    for key, selected in value.items():
        if key.endswith("_run"):
            if type(selected) is not bool:
                raise SelectionError("invalid platform execution flag")
            job = key[:-4].replace("_", "-")
            if job == "macos-first":
                expected = "macos" in pending and selection["macos_first"]
            else:
                expected = any(target.split("/")[0] == job for target in pending)
            if selected != expected:
                raise SelectionError("platform execution flag disagrees with pending targets")
        elif key.endswith("_matrix"):
            targets = [row["target"] for row in selected["include"]]
            job = key[:-7].replace("_", "-")
            if len(targets) != len(set(targets)) or set(targets) != {target for target in pending if target.split("/")[0] == job}:
                raise SelectionError("matrix disagrees with pending targets")
    return value


def prepare(selection, policy, jobs, event_name, event, fetch, repository, current_run):
    registry = json.loads(Path(".github/ci-platforms.json").read_text(encoding="utf-8"))
    identity, receipts = "", {}
    if event_name == "pull_request":
        tree = subprocess.check_output(["git", "rev-parse", "HEAD^{tree}"], text=True).strip()
        identity = fingerprint(event, tree)
        # Completion must fail on unreadable history, never silently run everything.
        # Normal PR waves are fresh and need no historical API queries.
        if completion_requested(event):
            receipts = previous_successes(fetch, repository, event["pull_request"]["head"]["sha"], identity, current_run)
            if pr_snapshot(fetch(f"repos/{repository}/pulls/{event['number']}")) != pr_snapshot(event["pull_request"]):
                raise SelectionError("PR changed while reading platform receipts")
    return plan(selection, policy, registry, jobs, event_name, event, identity, receipts)
