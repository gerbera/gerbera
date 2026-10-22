#!/usr/bin/env python3
"""Select expensive Web checks conservatively; keep the existing required js job."""

import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path


class SelectionError(ValueError):
    """The complete change set cannot be established safely."""


def safe_path(value):
    if (not isinstance(value, str) or not value or "\\" in value
            or any(ord(char) < 32 for char in value)
            or any(part in ("", ".", "..") for part in value.split("/"))):
        raise SelectionError("invalid changed-file path")
    return value


def classify(files):
    """Only documentation and the isolated Minimal workflow may omit Web tests."""
    if not isinstance(files, list) or not files:
        raise SelectionError("empty or invalid changed-file list")
    components = set()
    names = set()
    for change in files:
        if not isinstance(change, dict):
            raise SelectionError("invalid changed-file record")
        name = safe_path(change.get("filename"))
        if name in names:
            raise SelectionError("duplicate changed-file record")
        names.add(name)
        status = change.get("status")
        if status not in ("added", "modified", "removed", "renamed"):
            raise SelectionError("unrecognised changed-file status")
        paths = [name]
        if status == "renamed":
            paths.append(safe_path(change.get("previous_filename")))
        for path in paths:
            if path == ".github/workflows/minimal.yml":
                components.add("Minimal workflow")
            elif ((path.startswith("doc/") and path.endswith((".rst", ".md")))
                  or path in ("README.md", "ChangeLog.md", "ReleaseNotes.md")):
                components.add("documentation")
            elif path.startswith(("gerbera-web/", "web/")):
                components.add("Web")
            else:
                # Native/API, shared build/CI, this selector and unknown paths
                # retain Web coverage until a wider dependency map is agreed.
                components.add("native, shared or unclassified inputs")
    run_web = not components.issubset({"documentation", "Minimal workflow"})
    return run_web, ", ".join(sorted(components))


def pr_snapshot(pr):
    try:
        head, base, count = pr["head"]["sha"], pr["base"]["sha"], pr["changed_files"]
    except (KeyError, TypeError) as error:
        raise SelectionError("missing PR metadata") from error
    if (not isinstance(head, str) or not re.fullmatch(r"[0-9a-f]{40}", head)
            or not isinstance(base, str) or not re.fullmatch(r"[0-9a-f]{40}", base)
            or type(count) is not int or not 0 < count < 3000):
        raise SelectionError("invalid PR metadata or file-list limit reached")
    return head, base, count


def select(event_name, event, repository, fetch):
    if event_name != "pull_request":
        return True, "full Web validation for non-PR event"
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository):
        raise SelectionError("invalid repository")
    try:
        number = event["number"]
        event_head = event["pull_request"]["head"]["sha"]
        event_base = event["pull_request"]["base"]["sha"]
    except (KeyError, TypeError) as error:
        raise SelectionError("missing PR event metadata") from error
    if type(number) is not int or number <= 0:
        raise SelectionError("invalid PR number")
    endpoint = f"repos/{repository}/pulls/{number}"
    before = pr_snapshot(fetch(endpoint))
    if before[:2] != (event_head, event_base):
        raise SelectionError("PR head/base no longer matches this event")
    files = []
    # GitHub lists at most 3000 files. Do not accept a truncated list or a
    # changed PR while reading it, including movement of the base branch.
    for page in range(1, 31):
        rows = fetch(f"{endpoint}/files?per_page=100&page={page}")
        if not isinstance(rows, list) or len(rows) > 100:
            raise SelectionError("invalid files API response")
        files.extend(rows)
        if len(rows) < 100:
            break
    else:
        raise SelectionError("file-list pagination limit reached")
    if len(files) != before[2] or pr_snapshot(fetch(endpoint)) != before:
        raise SelectionError("incomplete file list or PR changed during selection")
    return classify(files)


def api_fetch(token):
    def fetch(path):
        request = urllib.request.Request(
            "https://api.github.com/" + path,
            headers={"Authorization": "Bearer " + token,
                     "Accept": "application/vnd.github+json",
                     "X-GitHub-Api-Version": "2022-11-28",
                     "User-Agent": "gerbera-ci-selection"},
        )
        with urllib.request.urlopen(request, timeout=20) as response:
            return json.load(response)
    return fetch


def execution_error(env):
    """Reject missing/failed dependencies before running the selected commands."""
    if (env.get("SELECTOR_TESTS_OUTCOME") != "success"
            or env.get("SELECTION_OUTCOME") != "success"):
        return "selection or selector tests did not succeed"
    selected = env.get("RUN_WEB")
    if selected not in ("true", "false"):
        return "missing or invalid Web selection"
    expected_node = "success" if selected == "true" else "skipped"
    if env.get("NODE_OUTCOME") != expected_node:
        return "Node setup result does not match the selected coverage"
    return None


def main():
    if sys.argv[1:] == ["--check-result"]:
        error = execution_error(os.environ)
        if error:
            print("Web validation blocked: " + error, file=sys.stderr)
            return 1
        if os.environ["RUN_WEB"] == "false":
            print("Web tests not applicable; expensive commands were not executed.")
        return 0
    if sys.argv[1:]:
        print("Usage: select_web_tests.py [--check-result]", file=sys.stderr)
        return 1
    try:
        event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
        run_web, reason = select(os.environ.get("GITHUB_EVENT_NAME", ""), event,
                                 os.environ.get("GITHUB_REPOSITORY", ""),
                                 api_fetch(os.environ.get("GH_TOKEN", "")))
    except (SelectionError, OSError, ValueError, KeyError) as error:
        # An API outage or unreadable event must increase coverage, never omit it.
        if isinstance(error, SelectionError):
            detail = str(error)
        elif isinstance(error, urllib.error.HTTPError):
            detail = f"HTTP {error.code}"
        else:
            detail = type(error).__name__
        run_web, reason = True, "full Web fallback: " + detail
    selected = "true" if run_web else "false"
    summary = ("### Web test selection\n\n"
               f"- Web tests: {'selected' if run_web else 'not applicable (not executed)'}.\n"
               f"- Reason: {reason}.\n"
               "- Other build/test matrices and CodeQL are unchanged.\n")
    print(summary)
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        output.write(f"web={selected}\n")
    with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as output:
        output.write(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
