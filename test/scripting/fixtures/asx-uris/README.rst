ASX preparation corpus
======================

This is a preparation-only corpus for issue #4007, based on ee79f479bc5bf0407c3fad59b438b6b4e7ad04ee.
No production importer changes, CTest registration or passing feature-test
claim are included. Expected lists are independent acceptance expectations.
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

Run ``python test/scripting/verify_asx_fixtures.py test/scripting/fixtures/asx-uris`` to check fixture
hashes, declared XML parsing outcomes, key references and nested local files.
That command validates test input consistency only, not Gerbera behavior.
