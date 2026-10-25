ASX playlist import
===================

Gerbera imports ASX version 3 as an ordered UPnP playlist. It does not emulate
Windows Media Player. XML declarations and byte-order marks determine the
document encoding; the legacy ``playlist-charset`` does not decode the same
XML metadata a second time.

Supported elements
------------------

.. list-table:: ASX server support
   :header-rows: 1
   :widths: 25 15 60

   * - Element or attribute
     - Support
     - Import behavior
   * - ``ASX``, ``ENTRY``, ``REF HREF``
     - Supported
     - Entries retain source order and explicit repetitions. Multiple REF
       children are alternatives for one position. Gerbera tries them in
       order until a local media object can be imported or an external
       reference can be represented. External availability is not probed;
       a renderer does not receive a playback-time rollover list.
   * - ``REPEAT COUNT``
     - Bounded
     - Only the first REPEAT in each document is processed. Its sequence is
       expanded COUNT times; 0 and 1 both mean one pass. Missing COUNT means
       unlimited player repetition and is imported once with a warning.
       Later or nested REPEAT blocks, including their contents, are ignored
       with a warning. Invalid counts or excessive expansion reject the plan.
   * - ``ENTRYREF HREF``
     - Local only
     - Insert the child document's entries at the reference position. Resolve
       its relative references from its own directory. Remote or missing
       children are skipped with a warning, without a network request.
       Malformed children, cycles and nesting-limit failures reject the plan.
   * - ``ENTRY SKIPIFREF``
     - Supported
     - ``YES`` skips an entry when its document is included through ENTRYREF.
       Opening that document directly retains the same entry.
   * - ``TITLE``
     - Partial
     - Use the first title in the entry scope. Embedded local media titles
       take precedence unless the existing explicit writeThrough extension
       requests an override. Root TITLE does not replace every media title
       or the filename-derived playlist container name.
   * - ``AUTHOR``, ``COPYRIGHT``, ``ABSTRACT``
     - Supported metadata
     - The first value in each scope is used. Entry values override document
       defaults. Each child document has its own defaults. AUTHOR and
       COPYRIGHT fill missing ``upnp:author`` and ``dc:rights`` in the virtual
       item; existing embedded values are retained. ABSTRACT supplies the
       virtual item's description. These mappings do not write media files.
   * - ``PARAM``
     - Gerbera extensions
     - Preserve ``mimetype``, ``protocol`` and ``size`` properties and other
       PARAM metadata. Document parameters supply defaults; entry parameters
       override them. Existing REF ``writeThrough`` database updates remain
       available and are applied in the first playlist view only.
   * - ``BASE HREF``
     - Gerbera compatibility
     - A root BASE prefixes relative media references; an entry BASE
       overrides it for that entry. Absolute references remain unchanged.
       This extends Gerbera's historical media-path behavior. Microsoft
       documents BASE for script-command URL flipping, not as a general
       media REF base. Gerbera does not execute URL-flip script commands.
   * - ``STARTTIME``, ``DURATION``, ``PREVIEWDURATION``, ``STARTMARKER``, ``ENDMARKER``
     - Unsupported
     - Report a warning. Resources contain the full original media and retain
       its actual duration. No clipping, marker selection or duration
       enforcement is advertised.
   * - ``EVENT``, ``BANNER``, ``MOREINFO``, ``LOGURL``, ``SKIN``, ``CLIENTSKIP``
     - Unsupported
     - Player events and presentation behavior cannot control a UPnP
       renderer. Event-contained entries are not inserted. No event,
       browser, banner or logging URL is executed. ``CLIENTSKIP=NO`` cannot
       prevent the renderer from skipping an item.

Expansion and update limits
---------------------------

At most five nested ENTRYREF documents below the root and 10000 expanded
positions are supported. A separate limit of 10000 ENTRYREF traversals also
bounds work for repeated or empty child documents. REPEAT COUNT must contain
decimal digits representing a value between 0 and
10000. The complete structural plan is validated before creating playlist
positions or applying writeThrough updates. A cycle is a document already
on the active canonical-path stack, including symbolic-link aliases.
Reusing a child after its earlier inclusion has returned is allowed.

ENTRYREF expansion uses the current child files when the parent playlist is
imported. After changing a referenced child, reimport the parent playlist to
refresh its expanded positions, for example by updating the parent's
modification time and rescanning it. A scan of unchanged parent files can
retain their existing positions. Child-file changes do not establish a new
automatic dependency-tracking system.

The limits and the one-pass handling of unbounded repetition are server
policies. They do not promise an unlimited or Windows Media Player runtime
playlist. Missing entries may be skipped, but structural errors do not
produce a partially expanded playlist.

References
----------

* `Microsoft REF alternatives <https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/ref-element>`_
* `Microsoft REPEAT <https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/repeat-element>`_
* `Microsoft ENTRYREF <https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/entryref-element>`_
* `Microsoft BASE scope <https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/base-element>`_
