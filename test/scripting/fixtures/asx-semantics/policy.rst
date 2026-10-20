ASX server import policy
========================

These decisions define the intended feature regression expectations. They do
not constitute a claim that the current importer passes those expectations.

Ordered alternatives
--------------------

Each ENTRY is one position. Try REF children in source order until a local
media object can be imported, or an external reference can be represented.
External availability is not probed and playback-time rollover is not
implemented. Preserve each alternative's own writeThrough attribute.

Bounded expansion
-----------------

Only the first REPEAT block in each document is considered; later blocks,
including their entries, are ignored with a diagnostic (case f14). COUNT=0
and COUNT=1 mean one pass. A missing COUNT is unbounded player behavior and is
represented by one pass with a warning. Negative, noninteger or excessive
counts reject the plan. The complete structural plan is validated before
playlist objects or writeThrough updates are created.

Local ENTRYREF children are inserted in source order using the child's own
directory. At most five nested references and 10000 expanded positions are
allowed. At most 10000 ENTRYREF traversals also bound empty child expansion.
Detect cycles using the active document stack, including canonical
paths so symlink aliases cannot evade the check. Reusing a child after it has
returned is not a cycle. A missing child is diagnosed and skipped; malformed
children, cycles and expansion-limit failures reject the complete plan.
Remote children are diagnosed and skipped without issuing network requests.
SKIPIFREF=YES applies only when the document is included through ENTRYREF.

Metadata and BASE
-----------------

Keep only the first TITLE, AUTHOR, COPYRIGHT and ABSTRACT in each scope.
Each document supplies its own AUTHOR, COPYRIGHT and ABSTRACT defaults;
ENTRY values override those defaults (case f25). A document TITLE describes
the playlist, not every clip. Preserve embedded media titles and metadata
unless existing explicit writeThrough behavior requests an override. PARAM
extensions retain their established behavior and do not leak into the next
ENTRY. No media file bytes are modified by these database operations.

Preserve Gerbera's existing root BASE compatibility for relative REF values.
An ENTRY BASE overrides the root BASE for that entry; an absolute reference
is unaffected (case f30). ENTRYREF context remains the referenced document's
own location. This media-path compatibility behavior is a Gerbera extension:
Microsoft documents BASE for player script-command URL flipping, which
Gerbera does not execute.

Player behavior outside server import
-------------------------------------

STARTTIME, DURATION, PREVIEWDURATION, STARTMARKER and ENDMARKER do not clip
resources and do not replace the actual media duration. EVENT entries,
BANNER, MOREINFO, LOGURL, SKIN and CLIENTSKIP cannot control a UPnP renderer.
No script, presentation, logging or remote child URL is executed. Emit
diagnostics for unsupported structural/player elements rather than claiming
Windows Media Player runtime compatibility.

Primary references
------------------

* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/ref-element
* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/repeat-element
* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/entryref-element
* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/base-element
* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/author-element
* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/copyright-element
* https://learn.microsoft.com/en-us/previous-versions/windows/desktop/wmp/abstract-element
