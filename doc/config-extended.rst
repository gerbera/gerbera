Configure Extended Runtime Options
==================================

.. index:: Extended Runtime Options

``extended-runtime-options``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~


::

    <extended-runtime-options>

These options reside under the server tag and allow to additinally control the so called "runtime options".
The difference to the import options is:

Import options are only triggered when data is being imported into the database, this means that if you want new
settings to be applied to already imported data, you will have to reimport it. Runtime options can be switched on
and off without the need to reimport any media. Unfortunately you still need to restart the server so that these
options take effect, however after that they are immediately active.

.. index:: FFMPEG Thumbnailer

``ffmpegthumbnailer``
~~~~~~~~~~~~~~~~~~~~~

::

    <ffmpegthumbnailer enabled="no">

* Optional
* Default: **no**

Ffmpegthumbnailer is a nice, easy to use library that allows to generate thumbnails from video files.
Some DLNA compliant devices support video thumbnails, if you think that your device may be one of those you
can try enabling this option. It would also make sense to enable the protocolInfo option, since it will
add specific DLNA tags to tell your device that a video thumbnail is being offered by the server.

The following options allow to control the ffmpegthumbnailer library (these are basically the same options as the
ones offered by the ffmpegthumbnailer command line application). All tags below are optional and have sane default values.

    ::

        <cache-dir enabled="yes">/home/gerbera/cache-dir</cache-dir>

    * Optional
    * Default: **<gerbera-home>/cache-dir**

    Storage location for the thumbnail cache when FFMPEGThumbnailer is enabled.  Defaults to Gerbera Home.
    Creates a thumbnail with file format as: ``<movie-filename>-thumb.jpg``

    The attributes of the tag have the following meaning:

    ::

            enabled=...

    * Optional
    * Default: **yes**

    Enables or disables the use of cache directory for thumbnails, set to ``yes`` to enable the feature.

    ::

        <thumbnail-size>128</thumbnail-size>

    * Optional
    * Default: **128**

    The thumbnail size should not exceed 160x160 pixels, higher values can be used but will mostprobably not be
    supported by DLNA devices. The value of zero or less is not allowed.

    ::

        <seek-percentage>5</seek-percentage>

    * Optional
    * Default: **5**

    Time to seek to in the movie (percentage), values less than zero are not allowed.

    ::

        <filmstrip-overlay>yes</filmstrip-overlay>

    * Optional
    * Default: **yes**

    Creates a filmstrip like border around the image, turn this option off if you want pure images.

    ::

        <image-quality>8</image-quality>

    * Optional
    * Default: **8**

    Sets the image quality of the generated thumbnails.

    ::

        <workaround-bugs>no</workaround-bugs>

    * Optional
    * Default: **no**

    According to ffmpegthumbnailer documentation, this option will enable workarounds for bugs in older ffmpeg versions.
    You can try enabling it if you experience unexpected behaviour, like hangups during thumbnail generation, crashes and alike.

.. index:: LastFM

``lastfm``
~~~~~~~~~~

::

    <lastfm enabled="no">

* Optional

Support for the last.fm service.

    ::

        <username>login</username>

    * Required

    Your last.fm user name.

    ::

        <password>pass</password>

    * Required

    Your last.fm password.

    ::

        <mark-played-items enabled="no" suppress-cds-updates="yes">

    * Optional

    The attributes of the tag have the following meaning:

    ::

        enabled=...

    * Optional
    * Default: **no**

    Enables or disables the marking of played items, set to ``yes`` to enable the feature.

    ::

        suppress-cds-updates=...

    * Optional
    * Default: **yes**

    This is an advanced feature, leave the default setting if unsure. Usually, when items are modified the system sends out
    container updates as specified in the Content Directory Service. This notifies the player that data in a particular
    container has changed, players that support CDS updates will rebrowse the container and refresh the view.
    However, in this case we probably do not want it (this actually depends on the particular player implementation).
    For example, if the system updates the list of currently playing items, the player could interrupt playback and rebrowse
    the current container - clearly an unwanted behaviour. Because of this, Gerbera provides an option to suppress and not
    send out container updates - only for the case where the item is marked as "played". In order to see the changes
    you will have to get out of the current container and enter it again - then the view on your player should get updated.

    Note:
        some players (i.e. PS3) cache a lot of data and do not react to container updates, for those players it may
        be necessary to leave the server view or restart the player in order to update content (same as when adding new data).

   **The following tag defines how played items should be marked:**

    .. code-block:: xml

        <string mode="prepend">* </string>

    * Optional
    * Default: **\\**

    Specifies what string should be appended or prepended to the title of the object that will be marked as "played".

        ::

            mode=...

        * Optional
        * Default: **prepend**

        Specifies how a string should be added to the object's title, allowed values are "append" and "prepend".

    ::

        <mark>

    * Optional

    This subsection allows to list which type of content should get marked.  It could also be used with audio and image content,
    but otherwise it's probably useless. Thefore Gerbera specifies only three supported types that can get marked:

    .. code-block:: xml

        <content>audio</content>
        <content>video</content>
        <content>image</content>

    You can specify any combination of the above tags to mark the items you want.
