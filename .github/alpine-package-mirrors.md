# Alpine package downloads in Minimal CI

The bootstrap host is pinned to `ubuntu-24.04`. A targeted run on the rolling
`ubuntu-latest` label selected Ubuntu 26.04, whose sudo implementation discarded
the environment needed by setup-alpine's `sudo -E` command. Pinning the host
avoids that incompatibility without changing sudo policies or the Alpine matrix.

The verified `apk.static` bootstrap download and the subsequent Alpine package
repositories are separate network operations. The former retains the CDN/GitLab
fallback introduced by #4018. Package indexes and packages use HTTPS repositories:

1. `https://dl-cdn.alpinelinux.org/alpine`
2. `https://mirrors.edge.kernel.org/alpine` (listed by Alpine as an official mirror)

Before `setup-alpine`, the selector downloads both `main` and `community` indexes
for the matrix's exact branch and architecture. Each download has a ten-second
connection timeout, a thirty-second transfer timeout and at most two retries with
five-second delays (100-second retry budget). The selection step is capped at
eight minutes. HTTP errors, invalid archives and indexes without package records
cause selection of the second mirror. No usable mirror means a failed job.

The selected base URL is passed through setup-alpine's existing `mirror-url`
input. When the primary is selected, the second mirror is also supplied through
`extra-repositories` for failures after the probe. If the primary fails the probe,
only the fallback is configured. Branch and architecture are never changed.

The probe checks availability and archive structure, not authenticity. The
existing apk index/package signature checks and trusted Alpine keys remain in
force. TLS certificate verification remains enabled. This cannot guarantee
availability during an outage affecting both mirrors or prevent every failure
between the probe and installation. Failure remains visible; build/test steps
are not retried, skipped, or allowed to fail.

The six selector regression tests run once in the existing x86/edge matrix job,
without another runner job. They cover fallback and terminal failures, malformed
indexes, matrix inputs and actual curl retries against an isolated TLS fixture
returning two HTTP 503 responses. The fixture also rejects an untrusted server
certificate. It uses temporary files and never changes the machine trust store.
