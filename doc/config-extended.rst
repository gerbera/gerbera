Configure Extended Runtime Options
==================================

.. index:: Extended Runtime Options

########################
Extended Runtime Options
########################

.. contents::
   :backlinks: entry
.. sectnum::
   :prefix: 1.
   :start: 6


.. confval:: extended-runtime-options
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <extended-runtime-options>
         <ffmpegthumbnailer>...</ffmpegthumbnailer>
         <lastfm>...</lastfm>
      </extended-runtime-options>

These options reside under the server tag and allow to additinally control the so called "runtime options".
The difference to the import options is:

Import options are only triggered when data is being imported into the database, this means that if you want new
settings to be applied to already imported data, you will have to reimport it. Runtime options can be switched on
and off without the need to reimport any media. Unfortunately you still need to restart the server so that these
options take effect, however after that they are immediately active.

.. index:: FFMPEG Thumbnailer

******************
FFMpeg Thumbnailer
******************

.. confval:: ffmpegthumbnailer
   :type: :confval:`Section`
   :required: false

   .. code:: xml

       <ffmpegthumbnailer enabled="yes"></ffmpegthumbnailer>

Ffmpegthumbnailer is a nice, easy to use library that allows to generate thumbnails from video files.
Some DLNA compliant devices support video thumbnails, if you think that your device may be one of those you
can try enabling this option.

.. confval:: ffmpegthumbnailer enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

       enabled="yes"

The following options allow to control the ffmpegthumbnailer library (these are basically the same options as the
ones offered by the ffmpegthumbnailer command line application). All tags below are optional and have sane default values.

.. confval:: ffmpegthumbnailer video-enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

       video-enabled="no"

Enables or disables the use thumbnails for videos, set to ``no`` to disable the feature.

.. confval:: ffmpegthumbnailer image-enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

      image-enabled="no"

Enables or disables the use thumbnails for images, set to ``no`` to disable the feature.

.. confval:: ffmpegthumbnailer cache-dir
   :type: :confval:`Path`
   :required: false
   :default: ``${gerbera-home}/cache-dir``

   .. code:: xml

      <cache-dir enabled="yes">/home/gerbera/cache-dir</cache-dir>

Database location for the thumbnail cache when FFMPEGThumbnailer is enabled. Defaults to Gerbera Home.
Creates a thumbnail with file format as: ``<movie-filename>-thumb.jpg``.

The attributes of the tag have the following meaning:

.. confval:: ffmpegthumbnailer cache-dir enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

      enabled="no"

Enables or disables the use of cache directory for thumbnails, set to ``yes`` to enable the feature.

.. confval:: ffmpegthumbnailer thumbnail-size
   :type: :confval:`Integer`
   :required: false
   :default: ``160``

   .. code:: xml

      <thumbnail-size>160</thumbnail-size>

The thumbnail size should not exceed 160x160 pixels, higher values can be used but will mostprobably not be
supported by DLNA devices. The value of zero or less is not allowed.

.. confval:: ffmpegthumbnailer seek-percentage
   :type: :confval:`Integer`
   :required: false
   :default: ``5``

   .. code:: xml

      <seek-percentage>5</seek-percentage>

Time to seek to in the movie (percentage), values less than zero are not allowed.

.. confval:: ffmpegthumbnailer filmstrip-overlay
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

      <filmstrip-overlay>yes</filmstrip-overlay>

Creates a filmstrip like border around the image, turn this option off if you want pure images.

.. confval:: ffmpegthumbnailer rotate
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

      <rotate>yes</rotate>

.. versionadded:: HEAD

Rotates the thumbnail depending on orientation of the movie/picture.

.. confval:: ffmpegthumbnailer image-quality
   :type: :confval:`Integer`
   :required: false
   :default: ``8``

   .. code:: xml

        <image-quality>5</image-quality>

Sets the image quality of the generated thumbnails.

.. index:: LastFM

*******
Last.FM
*******

.. confval:: lastfm
   :type: :confval:`Section`
   :required: false

   .. code:: xml

      <lastfm enabled="yes">

.. confval:: lastfm enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

       enabled="yes"

Support for the last.fm service. Gerbera has to be built with LastFM support.

.. confval:: lastfm username
   :type: :confval:`String`
   :required: true
   :default: `empty`

   .. code:: xml

        <username>login</username>

Your last.fm user name.

.. confval:: lastfm password
   :type: :confval:`String`
   :required: true
   :default: `empty`

   .. code:: xml

        <password>pass</password>

Your last.fm password.

*****************
Mark Played Items
*****************

.. confval:: mark-played-items
   :type: :confval:`Section`
   :required: false

   .. code:: xml

        <mark-played-items enabled="yes" suppress-cds-updates="yes">

The attributes of the tag have the following meaning:

.. confval:: mark-played-items enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``

   .. code:: xml

        enabled="yes"

Enables or disables the marking of played items, set to ``yes`` to enable the feature.

.. confval:: suppress-cds-updates
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``

   .. code:: xml

        suppress-cds-updates="no"

This is an advanced feature, leave the default setting if unsure. Usually, when items are modified the system sends out
container updates as specified in the Content Directory Service. This notifies the player that data in a particular
container has changed, players that support CDS updates will rebrowse the container and refresh the view.
However, in this case we probably do not want it (this actually depends on the particular player implementation).
For example, if the system updates the list of currently playing items, the player could interrupt playback and rebrowse
the current container - clearly an unwanted behaviour. Because of this, Gerbera provides an option to suppress and not
send out container updates - only for the case where the item is marked as "played". In order to see the changes
you will have to get out of the current container and enter it again - then the view on your player should get updated.

Note:
    Some players (i.e. PS3) cache a lot of data and do not react to container updates, for those players it may
    be necessary to leave the server view or restart the player in order to update content (same as when adding new data).

**The following tag defines how played items should be marked:**

.. confval:: mark-played-items string
   :type: :confval:`String`
   :required: false
   :default: ``"* "``

   .. code-block:: xml

       <string mode="prepend"># </string>

Specifies what string should be appended or prepended to the title of the object that will be marked as "played".

.. confval:: mark-played-items node
   :type: :confval:`Enum` (``prepend|append``)
   :required: false
   :default: ``prepend``

   .. code-block:: xml

            mode="append"

Specifies how a string should be added to the object's title, allowed values are "append" and "prepend".

.. confval:: mark
   :type: :confval:`Section`
   :required: false

   .. code:: xml

        <mark>

This subsection allows to list which type of content should get marked.  It could also be used with audio and image content,
but otherwise it's probably useless. Thefore Gerbera specifies only three supported types that can get marked:

.. confval:: content
   :type: :confval:`String`
   :required: true

   .. code-block:: xml

        <content>audio</content>
        <content>video</content>
        <content>image</content>

You can specify any combination of the above tags to mark the items you want.
