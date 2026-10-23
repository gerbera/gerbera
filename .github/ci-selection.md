# Component-aware CI, platform selection and incremental completion

Build/test groups share `.github/workflows/ci.yml`. One selection job reads the
complete PR change set and reports selected and not-applicable groups. CodeQL
keeps its separate workflow, both languages, configuration and existing triggers.

## Configuration

The default in `.github/ci-policy.json` is:

```json
{
  "macos_first": false,
  "platforms": ["*"]
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

## Select platforms before assigning runners

`platforms` is an allowlist of target identifiers or shell-style `*`/`?` patterns.
The default `['*']` retains every existing eligible platform. For example:

```json
{
  "macos_first": false,
  "platforms": ["cpp/gcc-13", "minimal/x86_64/v3.24", "minimal/aarch64/v3.24", "macos"]
}
```

This first PR wave runs those native platforms plus any affected Web, formatting
and XML checks. Other native compilers, operating systems and architectures are
listed as **excluded / NOT tested**. Jobs excluded by this policy are skipped
before matrix expansion and runner allocation. No placeholder build reports a
successful test. CodeQL remains independently enabled for both languages.

`.github/ci-platforms.json` is the single matrix registry. Identifiers include
`cpp/clang-18`, `head-check/check`, `head-check/npupnp`,
`minimal/ppc64le/edge`, `deb-package/ubuntu:24.04/linux/amd64`, `FreeBSD`,
`OmniOS`, `macos`, `docker-build` and `docker-debug`. Docker image jobs are atomic
targets; this setting does not split their internal image builds. The existing
Debian restriction to amd64 in PRs and the Alpine ppc64le/v3.24 exclusion remain.
Unknown patterns, an empty allowlist and malformed policies fail explicitly.
Master/tag/manual/scheduled runs ignore the partial allowlist and run full
coverage for their existing platform eligibility.

## Complete only missing platforms

After a partial wave finishes, a maintainer can add the PR label
**`ci:complete-platforms`**. Removing and re-adding that label requests another
completion pass. The label event queues behind the current wave rather than
cancelling the results it intends to reuse. Other label events use one short
control job to record that no build/test coverage was requested; their separate
concurrency group cannot displace a queued completion request. The completion
pass ignores the allowlist and executes only
affected targets without a verified successful result, including failed targets.
Successful Web/formatting/XML jobs are reused too. CodeQL is not relaunched by
this label event; its separate existing checks remain authoritative.

Reuse is restricted to the same PR number, head, base and complete tested merge
tree. The tree includes the policy, platform registry, workflow and dependency
recipes. Changing any of these invalidates old receipts. Moving the base also
invalidates them. Each scheduled job carries that fingerprint and its target ID
in a completion step and records success only after its actual build/test payload
succeeds. The selector reads every workflow/job page and attempt, with the latest
executed result taking precedence. A cancelled/failed job, a skipped Mac path or
a guard-only success is not successful coverage. The summary links each reused
job to its original run. Results from older workflow versions without these
receipts cannot be reused.

Active runs/jobs, unreadable or unexpectedly empty job lists, API failures and
pagination limits block completion. A run/job explicitly cancelled before any
step executed provides no receipt; it cannot deadlock later completion after
GitHub replaces a pending concurrency request. A missing-only request never silently becomes a fresh
full matrix after an API failure. No old job/check is rewritten to green.
Partial coverage remains explicitly partial; the `lint` result verifies the
requested wave and does not claim that excluded platforms passed. Maintainers
must require completion before merge when full platform coverage is needed.
Job/check names retain their existing names. If branch protection requires an
excluded platform's individual context, it remains missing until that platform
runs; it is never synthesized from another platform's result. This PR does not
alter branch protection or bypass existing checks.

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
- Compiler, Minimal and Debian payloads, eligible variants and check names remain;
  a completion step holds the exact-tree fingerprint. A not-applicable
  group or excluded variant allocates no build/test runner. Measure representative
  runs before claiming a particular runner-minute saving.
- The required **`lint`** context becomes the completion/formatting gate. It
  checks every group against the selection and chosen Mac path. Failed,
  cancelled, missing or unexpectedly skipped selected results fail the gate;
  formatting failures remain blocking. CodeQL keeps its independent required
  `Analyze` contexts.

No workflow-level path filters, `continue-on-error` or automatic retries are
added. A failed macOS-first run blocks selected downstream build commands and
fails completion. Cancellation does not start downstream build/test commands.
The short completion gate still checks cancelled or missing selected results and fails;
it does not treat an unused gate as a successful validation.

## Complete changes and full runs

PR-files pages, both rename paths and deletions are checked. Head/base/file count
must agree before and after collection. Unknown paths, API errors, incomplete
lists, races or the 3000-file limit retain all component groups. An explicitly
configured partial platform policy still applies to a normal first wave;
completion API errors fail instead of silently expanding coverage. The read-only selection
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
The merged #4027 Alpine package-mirror selector, its regression check, pinned
bootstrap hashes and mirror inputs are retained in the central Minimal job.
