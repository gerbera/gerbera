ASX URI regression corpus
=========================

This corpus covers issue #4007. Independent acceptance expectations are consumed
by native ``AsxImportTest`` GoogleTest cases in ``testcontent``. They run all 39
documents with mt/grb and UTF-8/windows-1252, both playlist views, decoded XML
text, unchanged external MMS/RTSP URLs, and a later legacy PLS import.
Rescan and restart reopen the same isolated SQLite database. Separately named
HTTP cases compare complete server-delivered resources when CURL is available;
without CURL only those HTTP cases are explicitly skipped. The native importer
uses the existing Server, ConfigManager and database owners synchronously.
No extra Gerbera process, READY log or Python readiness runtime is needed.

Run the registered individual GoogleTest cases with CTest diagnostics:

``ctest --test-dir build -R AsxImportTest --output-on-failure``

Actual media files must be generated in an isolated Linux/macOS fixture from
``cases.json``; filenames containing ?, quotes or # must not be created in the
Windows checkout. Media tags are fixture-owned synthetic data, not library edits.

Run all cases in mt/grb modes and with UTF-8/windows-1252 legacy playlist charset,
and compare both playlist views. Check exact order, repeats, UTF-8 titles and
complete Gerbera-served bytes for local media. A file: resource passed through
as an external URL does not satisfy a local-media expectation. HTTP(S) queries
must remain byte-identical and no remote availability probe may be performed.

``required`` expectations describe acceptance/regression cases.
``proposed_*`` expectations are explicit Gerbera compatibility/server policies
for review, not assertions of Microsoft player behavior. ``requires_decision``
cases deliberately have no golden sequence yet. Resolve those policies before
registering importer tests; never turn these into passing no-op tests.

The manifest retains the original corpus SHA256 values for source audits.
Input hashes alone do not certify importer behavior; the native tests compare
the declared positions and actual media resources.

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
The separate ASX feature corpus belongs to issue #4008, not this bug fix.
