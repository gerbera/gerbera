WPL and XSPF preparation for Gerbera issue #3997
https://github.com/gerbera/gerbera/issues/3997

Status: input corpus and expected behavior only. Gerbera does not yet support
these formats on this branch. These files are not registered as passing tests.
Production servers and the shared music collection have not been changed.

cases.json contains the intended Linux import result for each input. The keys
A/B/V/U/Q/P/H/D identify synthetic media which the future integration fixture
must create under /fixture/xml/media. The playlist directory is /fixture/xml/pl.
HTTPS and RELATIVE_HTTPS identify external items; parsing must not download them.
No external URI in this corpus contains a real service or credential.

The corpus covers:
- static WPL media order, repetition, relative Windows/POSIX paths, absolute
  POSIX paths, file URIs, UTF-16, XML escaping and Unicode;
- rejecting inappropriate Windows drive/UNC paths on a Linux server, without
  inventing a path or mounting a share;
- XSPF namespace prefixes, namespace rebinding and ignored foreign extensions;
- inherited XML Base and one selected location per track (alternative sources
  must not create extra positions);
- URI decoding exactly once, literal plus signs and remote query preservation;
- missing media, empty playlists, malformed XML, wrong roots/namespaces and DTDs;
- explicit unsupported handling of dynamic WPL smartPlaylist queries.

Each case's expected sequence is an implementation target, not a test result.
The stdlib_parseable flag describes Python ElementTree's parsing of the fixture;
it is not XML/XSPF schema validation. In particular x16 contains an external
entity deliberately unsupported by ElementTree. Gerbera must reject the DTD
without reading its local sentinel or fetching an external resource.

Follow-up tests must call the actual Gerbera importer, compare both generated
playlist views and resources, and exercise changes/rescans/restarts. Preserve
existing M3U/M3U8/PLS/ASX behavior. Once #4006 is merged, include its update and
deletion regression scenarios for WPL/XSPF too; keep this feature branch separate.

Format references:
https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/media-element
https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/seq-element
https://xspf.org/spec
https://pugixml.org/docs/manual.html
