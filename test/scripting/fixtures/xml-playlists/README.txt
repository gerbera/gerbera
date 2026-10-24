WPL and XSPF fixtures for Gerbera issue #3997
https://github.com/gerbera/gerbera/issues/3997

The corpus is exercised by native XmlPlaylistTest cases and by the real-server
XmlPlaylistIntegration test. Production servers and the shared music collection
are not used or changed by these fixtures.

cases.json contains the intended Linux import result for each input. The keys
A/B/V/U/Q/P/H/D/R/K/L identify synthetic media created by the integration fixture.
/fixture/xml is replaced by its isolated temporary directory. The playlist directory is /fixture/xml/pl.
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

Each case declares its expected sequence independently of the importer.
The stdlib_parseable flag describes Python ElementTree's parsing of the fixture;
it is not XML/XSPF schema validation. In particular x16 contains an external
entity deliberately unsupported by ElementTree. Gerbera must reject the DTD
without reading its local sentinel or fetching an external resource.

The integration test calls the actual importer and compares both playlist
views and complete media resources, including repeated rescans and restart.
M3U/M3U8/PLS/ASX remain comparison formats. General source-update behavior is
tracked separately in #4005/#4006; combined update tests must identify that base.

Critical filenames cover literal + % # ? & = ; , : @ [ ] { } ( ) ! $ quotes,
Unicode including a supplementary code point, and a URI-encoded backslash.
The 896-byte video.mp4 is an original synthetic red 16x16 MPEG-4 test clip:
ffmpeg -f lavfi -i color=c=red:s=16x16:r=1 -t 1 -c:v mpeg4 -an video.mp4
It is distributed under the repository GPLv2 license. No external media used.

Format references:
https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/media-element
https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/seq-element
https://xspf.org/spec
https://pugixml.org/docs/manual.html
