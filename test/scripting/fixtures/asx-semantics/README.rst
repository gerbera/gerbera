ASX structure regression corpus
===============================

This corpus describes issue #4008. Expected lists are independent acceptance
expectations; runtime results are recorded separately from input validation.
Actual media files must be generated in an isolated Linux/macOS fixture from
``cases.json``; filenames containing ?, quotes or # must not be created in the
Windows checkout. Media tags are fixture-owned synthetic data, not library edits.

Run all cases in mt/grb modes and with UTF-8/windows-1252 legacy playlist charset,
and compare both playlist views. Check exact order, repeats, UTF-8 titles and
complete Gerbera-served bytes for local media. A file: resource passed through
as an external URL does not satisfy a local-media expectation. HTTP(S) queries
must remain byte-identical and no remote availability probe may be performed.

``required`` expectations describe acceptance/regression cases.
``documented_server_policy`` expectations are explicit Gerbera compatibility
and server policies, not assertions of Microsoft player runtime behavior.
The f14/f25/f30 decisions are defined in ``policy.rst`` and the manifest.

The manifest retains the original corpus SHA256 values for source audits.
These describe input consistency, not runtime outcomes. Native GoogleTest cases
in ``testcontent`` compare the importer result against the manifest.

Architecture after review of #4004
----------------------------------

Implement in this order: shared file/parser foundation (#4004), align WPL/XSPF
and expose reusable URI helpers (#4009), ASX URI/metadata bug fix (#4007), then
ASX structural extensions (#4008). The playlist-update fix (#4006) remains a
separate change and a combined-regression requirement; no additional reader or
configuration policy was found there that needs this architectural refactor.

Use the existing GrbFile APIs for disk access and a common ParserScript setup
and converter lifetime. Do not add a separate ASX file reader or duplicate the
WPL/XSPF URI decoder in JavaScript. Retain format-specific validation policies;
XML encoding declarations/BOMs must not be treated as opt-in M3U charset modes.
Existing GrbFile binary reading may use a stream internally; reusing that API
is not a claim that all existing GrbFile operations share one FILE handle.

Resolve common dependencies on a local integration base while PRs are unmerged.
Do not copy the same infrastructure into multiple PRs or include unrelated
feature commits in the ASX bug-fix diff. Rebase dependent work on the accepted
upstream foundation, or explicitly identify a stacked PR dependency.

Test parser/converter reuse after a valid ASX document, malformed input and a
subsequent legacy PLS import. Compare metadata, ordered positions and actual
media resources, rather than adding a unit test for each configuration default.
Native ``AsxImportTest`` cases run all 35 documents against the real importer
with mt/grb, both legacy charsets, both views and full local HTTP resources.
The ordinary rescan/restart cases first index the isolated media library before
adding ASX documents. Separately named inotify cases start media and playlists
together, then exercise parent/media/child edits, unchanged writable close,
restart, emptying and deletion. Those cases wait for the owned process's actual
kernel watches before changing files; they require Linux inotify. HTTP byte
cases require CURL and are explicitly named/skipped independently of DB cases.
A small local connection trap proves that remote child/presentation/logging
references and remote media are not probed during import. No external server
availability is required.

``ctest --test-dir build -R AsxImportTest --output-on-failure``
