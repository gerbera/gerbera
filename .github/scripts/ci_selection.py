#!/usr/bin/env python3
"""Dependency-aware CI selection and the final required result."""

import json
import os
from pathlib import Path
import subprocess
import sys
import urllib.error

from select_web_tests import SelectionError, api_fetch, safe_path, select


GROUPS = ("web", "native", "head", "freebsd", "omnios", "macos", "minimal",
          "deb", "docker", "xml", "format_cpp", "format_cmake", "security")
NATIVE = ("web", "native", "head", "freebsd", "omnios", "macos", "minimal", "deb", "docker")
JOBS = {"js": "web", "cpp": "native", "head-check": "head", "FreeBSD": "freebsd",
        "OmniOS": "omnios", "macos": "macos", "minimal": "minimal",
        "deb-package": "deb", "docker-build": "docker", "docker-debug": "docker",
        "xml": "xml", "format": "format", "macos-first": "macos"}


def full(reason):
    return {"groups": {**dict.fromkeys(GROUPS, True), "format": True},
            "components": ["all"], "reason": reason, "all_cmake": True,
            "cmake_files": [], "macos_first": False}


def cmake_source(path):
    return path.split("/")[-1] == "CMakeLists.txt" or (
        path.endswith(".cmake") and path.startswith(("cmake/", "test/")))


def classify_groups(files):
    # Validate all records/rename paths before interpreting any exemption.
    from select_web_tests import classify
    classify(files)
    paths = []
    for change in files:
        paths.append(change["filename"])
        if change["status"] == "renamed":
            paths.append(change["previous_filename"])
    def all_inputs(reason):
        selected = full(reason)
        selected["cmake_files"] = sorted({path for path in paths if cmake_source(path)})
        return selected
    flags = dict.fromkeys(GROUPS, False)
    flags["security"] = True  # Preserve both required CodeQL configurations.
    components, cmake_files = set(), set()
    all_cmake = False
    workflow_groups = {
        "ci-js.yml": ("web",), "macos.yml": ("macos",), "minimal.yml": ("minimal",),
        "publish-deb.yml": ("deb",), "docker.yml": ("docker",), "xml.yml": ("xml",),
        "clang-format.yml": ("format_cpp", "format_cmake"),
        "codeql-analysis.yml": ("security",),
    }
    for path in paths:
        affected = ()
        if path.startswith((".github/scripts/", "test/ci/")) or path in (
                ".github/workflows/ci.yml", ".github/ci-selection.md",
                ".github/cmake-format-files.json", ".github/ci-policy.json"):
            return all_inputs("shared CI controller or dependency policy changed")
        if path.startswith(".github/workflows/") and path.split("/")[-1] in workflow_groups:
            affected = workflow_groups[path.split("/")[-1]]
            components.add("workflow configuration")
            all_cmake = all_cmake or "format_cmake" in affected
        elif path.startswith(".github/codeql/"):
            components.add("security configuration")
        elif ((path.startswith("doc/") and path.endswith((".rst", ".md")))
              or path in ("README.md", "README.Docker.md", "ReleaseNotes.md")):
            components.add("documentation")
        elif path in ("ChangeLog.md", "ChangeLog", "doc/gerbera.1"):
            affected = ("head", "deb")
            components.add("packaged documentation")
        elif path.startswith("gerbera-web/"):
            affected = ("web",)
            components.add("Web tests and development dependencies")
        elif path.startswith("web/"):
            affected = ("web", "deb", "docker")
            components.add("Web assets and their packages")
        elif path.startswith("src/"):
            affected = NATIVE
            if path.startswith("src/config/"):
                affected += ("xml",)
                components.add("configuration")
            elif path.startswith("src/database/"):
                components.add("database")
            elif path.startswith(("src/content/", "src/metadata/")):
                components.add("import and metadata")
            else:
                components.add("native/server and API contracts")
            if path.endswith((".h", ".cc")) and not path.startswith("src/contrib/"):
                flags["format_cpp"] = True
        elif path.startswith("scripts/js/"):
            affected = NATIVE
            components.add("import scripts and native integration")
        elif path.startswith("test/") and not cmake_source(path):
            affected = NATIVE
            if path.startswith("test/config/"):
                affected += ("xml",)
                components.add("configuration tests")
            elif path.startswith("test/database/"):
                components.add("database tests")
            elif path.startswith("test/scripting/"):
                components.add("import script tests")
            else:
                components.add("native tests and shared fixtures")
        elif path.startswith("config/"):
            affected = NATIVE + ("xml",)
            components.add("configuration and schema")
        elif path.startswith(("scripts/debian/", "scripts/ubuntu/")):
            affected = ("deb", "head")
            components.add("Debian packaging and HEAD library builds")
        elif path.startswith("scripts/sunos/"):
            affected = ("omnios",)
            components.add("OmniOS platform")
        elif path.startswith("scripts/docker/") or path in ("Dockerfile", "Dockerfile.debug", ".dockerignore"):
            affected = ("docker",)
            components.add("Docker packaging")
        elif path.startswith("scripts/alpine/"):
            affected = ("minimal", "docker")
            components.add("Alpine platform and Docker dependencies")
        elif path == ".clang-format":
            affected = ("format_cpp",)
            components.add("C++ format policy")
        elif path == ".cmake-format.py":
            affected = ("format_cmake",)
            all_cmake = True
            components.add("CMake format policy")
        else:
            # Build files/presets, dependency installers, shared headers and any
            # unclassified path retain all consumers, including macOS.
            return all_inputs("shared build/dependency or unclassified input")
        for group in affected:
            flags[group] = True
        if cmake_source(path):
            flags["format_cmake"] = True
            cmake_files.add(path)
    flags["format"] = flags["format_cpp"] or flags["format_cmake"]
    return {"groups": flags, "components": sorted(components),
            "reason": ", ".join(sorted(components)), "all_cmake": all_cmake,
            "cmake_files": sorted(cmake_files), "macos_first": False}


def validate_selection(selection):
    if not isinstance(selection, dict) or set(selection.get("groups", {})) != set(GROUPS) | {"format"}:
        raise SelectionError("missing or unknown coverage group")
    if any(type(value) is not bool for value in selection["groups"].values()):
        raise SelectionError("coverage flags must be booleans")
    if selection["groups"]["format"] != (selection["groups"]["format_cpp"] or selection["groups"]["format_cmake"]):
        raise SelectionError("inconsistent formatting selection")
    if selection["groups"]["security"] is not True:
        raise SelectionError("required CodeQL coverage must remain selected")
    if type(selection.get("macos_first")) is not bool:
        raise SelectionError("macOS-first policy must be a boolean")
    if type(selection.get("all_cmake")) is not bool or not isinstance(selection.get("cmake_files"), list):
        raise SelectionError("invalid CMake file selection")
    for path in selection["cmake_files"]:
        safe_path(path)
        if not cmake_source(path):
            raise SelectionError("not a CMake source file")
    return selection


def macos_first(policy, event_name, event):
    if not isinstance(policy, dict) or set(policy) != {"macos_first"} or type(policy["macos_first"]) is not bool:
        raise SelectionError("invalid repository CI policy")
    order = event.get("inputs", {}).get("execution-order", "repository-default") if event_name == "workflow_dispatch" else "repository-default"
    if order not in ("repository-default", "parallel", "macos-first"):
        raise SelectionError("invalid execution order")
    return policy["macos_first"] if order == "repository-default" else order == "macos-first"


def final_errors(needs):
    if not isinstance(needs, dict) or set(needs) != set(JOBS) | {"selection"}:
        return ["missing or unexpected CI job result"]
    if needs["selection"].get("result") != "success":
        return ["coverage selection or its tests did not succeed"]
    try:
        selection = validate_selection(json.loads(needs["selection"]["outputs"]["selection"]))
        for group, selected in selection["groups"].items():
            if needs["selection"]["outputs"].get(group) != str(selected).lower():
                raise SelectionError("coverage outputs do not agree")
        if needs["selection"]["outputs"].get("macos_first") != str(selection["macos_first"]).lower():
            raise SelectionError("execution order outputs do not agree")
    except (KeyError, TypeError, ValueError) as error:
        return ["invalid coverage selection: " + str(error)]
    errors = []
    for job, group in JOBS.items():
        status = needs[job].get("result")
        selected = selection["groups"][group]
        if job in ("macos", "macos-first"):
            selected = selected and (selection["macos_first"] == (job == "macos-first"))
            if not selected and status != "skipped":
                errors.append(job + ": unused macOS path was not skipped")
                continue
        if selected:
            if status != "success":
                errors.append(job + ": selected but " + str(status))
        elif status not in ("success", "skipped"):
            errors.append(job + ": unexpected " + str(status))
    return errors


def format_cmake(selection, run=subprocess.run):
    validate_selection(selection)
    if selection["all_cmake"]:
        files = sorted(set(json.loads(Path(".github/cmake-format-files.json").read_text()))
                       | set(selection["cmake_files"]))
    else:
        files = selection["cmake_files"]
    failed = False
    for path in files:
        safe_path(path)
        if not cmake_source(path):
            raise SelectionError("invalid CMake source")
        if not Path(path).is_file():
            print("CMake format not applicable to deleted/moved file: " + path)
            continue
        print("Checking CMake format: " + path, flush=True)
        result = run(["cmake-format", "-c", "./.cmake-format.py", "--check", "./" + path], check=False)
        failed = failed or result.returncode != 0
    return 1 if failed else 0


def main():
    if sys.argv[1:] == ["--final-result"]:
        try:
            errors = final_errors(json.loads(os.environ["NEEDS_JSON"]))
        except (KeyError, TypeError, ValueError):
            errors = ["unreadable CI results"]
        if errors:
            print("CI completion failed:\n" + "\n".join(errors), file=sys.stderr)
            return 1
        print("Selected CI groups succeeded; groups not applicable were not executed.")
        return 0
    if sys.argv[1:] == ["--format-cmake"]:
        return format_cmake(json.loads(os.environ["SELECTION_JSON"]))
    if sys.argv[1:]:
        raise SelectionError("unknown CI selection command")
    event_name = os.environ.get("GITHUB_EVENT_NAME", "")
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
    policy = json.loads(Path(".github/ci-policy.json").read_text(encoding="utf-8"))
    first = macos_first(policy, event_name, event)
    try:
        if event_name == "pull_request":
            selection = select(event_name, event, os.environ.get("GITHUB_REPOSITORY", ""),
                               api_fetch(os.environ.get("GH_TOKEN", "")), classify_groups)
        else:
            selection = full("full validation for master push, tag, manual or scheduled event")
        validate_selection(selection)
    except (SelectionError, OSError, ValueError, KeyError) as error:
        detail = str(error) if isinstance(error, SelectionError) else type(error).__name__
        if isinstance(error, urllib.error.HTTPError):
            detail = "HTTP " + str(error.code)
        selection = full("full fallback: " + detail)
    selection["macos_first"] = first
    summary = "### CI coverage selection\n\n" + selection["reason"] + "\n\n| Group | Decision |\n| --- | --- |\n"
    for group, selected in selection["groups"].items():
        summary += f"| {group} | {'selected' if selected else 'not applicable / not executed'} |\n"
    summary += ("\nBuild order: " + ("macOS first when selected" if first else "parallel")
                + ". Formatting and the separate CodeQL workflow are independent of this order. CodeQL triggers and both languages are unchanged.\n")
    print(summary)
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        for group, selected in selection["groups"].items():
            output.write(f"{group}={str(selected).lower()}\n")
        output.write("selection=" + json.dumps(selection, separators=(",", ":")) + "\n")
        output.write("macos_first=" + str(first).lower() + "\n")
    with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as output:
        output.write(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
