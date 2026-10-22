# Web test selection (first stage of #4020)

The `CI validation for web UI` workflow retains its existing required `js` job.
Selection is a step inside that job, not an additional runner job or a
workflow-level path filter. The job summary records the decision and its reason.
Tests that are not applicable are explicitly reported as **not executed**.

| Complete PR change set | Expensive Web checks |
| --- | --- |
| Only `.rst` / `.md` under `doc/`, or root `README.md`, `ChangeLog.md`, `ReleaseNotes.md` | Not applicable |
| Only `.github/workflows/minimal.yml`, optionally with the documentation above | Not applicable |
| `gerbera-web/` or `web/` | Selected |
| Native/server/API, shared build/dependencies, other CI files, selector or its tests | Selected conservatively |
| Unknown paths, empty/incomplete file lists, API errors, PR changes during selection | Selected conservatively |

Both sides of a rename are classified. Deleted files are classified by their old
path. Documentation build inputs such as `doc/CMakeLists.txt`, configuration,
images and scripts are not covered by the prose-only exemption. This first
stage changes **only Web selection**; native, packaging, Minimal, documentation
and security workflows retain all their existing triggers and checks.

The selector reads every page of the PR files API and checks the head, base and
file count before and after pagination. At GitHub's 3000-file limit it chooses
full Web coverage. It does not fetch or execute names supplied by the PR as code.
The token is read-only and is passed only to the selection step.

Selector unit tests run on every execution. Unless the workflow is cancelled,
the existing `run tests` step always checks their outcome, selection output and
Node setup outcome before deciding
whether to execute the original npm commands. Missing results, failures,
cancellations or unexpected dependency skips cannot make selected tests pass.
Selected npm failures still fail the same `js` job; workflow cancellation does
not produce a successful required check. No `continue-on-error` is used.
The selection step has a two-minute limit; reaching it fails the check explicitly.

Pushes to `master` and manual `workflow_dispatch` runs always select all Web
checks. The selector also defaults to full Web coverage for any future non-PR
event; this PR does not add a schedule or change release workflows. Draft PRs
use the same rules as ready PRs. A deliberately published `[skip ci]` revision
does not run this workflow and still needs regular required checks before merge.

Run the controller tests without npm or a browser:

```sh
python3 -B -m unittest discover -s test/ci -p 'test_*.py'
```

## Scope and follow-up

This is an intentionally small initial dependency map, not a claim that native
changes are independent of the Web API. Extend exemptions only with evidence
and regression tests. CodeQL remains untouched. Broader matrix selection,
periodic full-suite policy and measured runner-minute savings remain part of
#4020. Compare representative real Actions runs before quantifying savings;
the small selection step still uses the existing Web job's runner.

#4019 changes this same workflow. Its job timeout, mock tests and browser failure
diagnostics must be retained when integrating these changes after that PR lands.
