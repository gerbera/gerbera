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

Run ``python test/scripting/verify_asx_fixtures.py test/scripting/fixtures/asx-semantics`` to check fixture
hashes, declared XML parsing outcomes, key references and nested local files.
That command validates test input consistency only, not Gerbera behavior.

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
The manifest verifier still checks inputs only. ``AsxSemanticsIntegration``
runs this corpus against the real importer with mt/grb, both legacy charsets,
both views and complete local resources. Its default mode first indexes the
isolated media library, then adds the ASX documents. The separate
``--exercise-media-updates`` mode starts media and playlists together and
adds inotify, parent/child/media edits, emptying and deletion for combined
validation with the independent playlist-update repair.
