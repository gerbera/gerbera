# Component-aware CI and optional macOS-first scheduling

Build/test groups share `.github/workflows/ci.yml`. One selection job reads the
complete PR change set and reports selected and not-applicable groups. CodeQL
keeps its separate workflow, both languages, configuration and existing triggers.

## Configuration

The default in `.github/ci-policy.json` is:

```json
{
  "macos_first": false
}
```

`false` keeps selected groups parallel. `true` runs the selected macOS build and
tests first; other expensive groups start only after its success. Formatting and
CodeQL run in parallel in either mode. If macOS is not affected, other groups
proceed without a Mac build. The two Mac paths reuse identical anchored steps;
exactly one path executes.

A manual **CI validation** run offers **Build order**: `repository-default`,
`parallel` or `macos-first`. Manual choices override the file for that run.
Automatic PR/master/tag/scheduled runs use the file. Invalid policy fails
selection explicitly; policy changes themselves select all groups.
The ordering option does not launch CodeQL: its existing separate triggers are
unchanged. When triggered, CodeQL never waits for the macOS scheduling gate.

## Dependency map

| Changed inputs | Selected consumers, in addition to CodeQL |
| --- | --- |
| Prose `.rst`/`.md` under `doc/`, root README/ReleaseNotes | No expensive build/test group |
| Packaged changelog or `doc/gerbera.1` | Debian packages and HEAD libraries |
| `gerbera-web/` tests, mocks, development dependencies | Web lint/component/browser checks |
| Deployed `web/` assets | Web checks, Debian packages, Docker |
| Native/server sources, import JavaScript, native tests | Native compilers, HEAD libraries, macOS, FreeBSD, OmniOS, Minimal, packaging and conservative Web coverage |
| Configuration sources, schema or configuration tests | Native consumers above plus XML schema checks |
| `scripts/debian/`, `scripts/ubuntu/` | Debian packages and HEAD libraries |
| `scripts/sunos/` | OmniOS |
| Dockerfiles, `.dockerignore`, `scripts/docker/` | Docker |
| `scripts/alpine/` | Minimal and Docker |
| `.clang-format` | C++ formatting |
| `.cmake-format.py` | All registered CMake formatting inputs |
| Shared CMake/build presets/dependencies, CI controller/policy, unknown paths | All groups, including macOS |

Configuration, import and database are functional components selecting their
native consumers. Their complete existing C++ suites remain enabled. No new
language-standard matrix or test-case-level CTest filter is introduced: all six
compiler variants, C++17 default and conditional C++20 PostgreSQL sources remain.
Shared CMake modules, dependency discovery and presets always retain macOS.
Legacy individual workflow paths map to their respective groups. Documentation
scripts/templates and unknown inputs are not covered by the prose exemption.

## Grouping and merge protection

- One **Formatting** job replaces the previous C++ job plus 40 CMake jobs. The
  existing C++ action/settings are retained. CMake inputs run on one runner;
  `.github/cmake-format-files.json` retains all 40 original files. Changed/new
  CMake files are included too; deleted paths are reported as not applicable.
- One XML job retains all nine original fixture/schema pairs and validation
  action, replacing their nine-runner matrix.
- Compiler, Minimal and Debian matrix names and variants remain, including the
  required contexts. A not-applicable group performs only a short guard, with
  no build/test commands. These compatibility checks still allocate some
  runners. Measure real representative runs before claiming runner-minute gains.
- The required **`lint`** context becomes the completion/formatting gate. It
  checks every group against the selection and chosen Mac path. Failed,
  cancelled, missing or unexpectedly skipped selected results fail the gate;
  formatting failures remain blocking. CodeQL keeps its independent required
  `Analyze` contexts.

No workflow-level path filters, `continue-on-error` or automatic retries are
added. A failed macOS-first run blocks selected downstream build commands and
fails completion. Cancellation does not start downstream build/test commands.
The short completion gate still checks cancelled or missing results and fails;
it does not treat an unused gate as a successful validation.

## Complete changes and full runs

PR-files pages, both rename paths and deletions are checked. Head/base/file count
must agree before and after collection. Unknown paths, API errors, incomplete
lists, races or the 3000-file limit retain full coverage. The read-only selection
token is used only in that job. Invalid policy or selection timeout fails.

Master pushes, version tags, manual CI runs and the weekly schedule select all
groups. Drafts use the same rules as ready PRs. `[skip ci]` suppresses automatic
CI; normal required checks must still run before merge. Standalone Web checking
remains available manually. Release metadata workflows are unchanged.

## Validation and integration

```sh
python3 -B -m unittest discover -s test/ci -p 'test_*.py'
```

Also lint workflow syntax and exercise actual shell guards for both orders,
preserved matrix/command payloads, formatter/XML batching, and failing/cancelled/
missing/skipped outcomes. Controller checks do not replace actual Mac, native,
browser and packaging runs before merge.

#4019 is merged. This Draft is based on master containing that change: its
15-minute Web limit, mock tests and browser failure diagnostics are retained in
both the central Web job and standalone manual Web workflow. The mock command
uses #4019's package/test files. CodeQL remains unchanged in its own workflow.
The obsolete `install` input of `docker/setup-buildx-action@v4` is omitted; image
build commands continue to use the existing build/push action. Refs #4020.
