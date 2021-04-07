.. index:: Transcoding Configuration

Configure Transcoding
=====================

The transcoding section allows to define ways on how to transcode content.

``transcoding``
~~~~~~~~~~~~~~~

.. code-block:: xml

    <transcoding enabled="yes" fetch-buffer-size="262144" fetch-buffer-fill-size="0">

* Optional

This tag defines the transcoding section.

**Attributes:**

    ::

        enabled=...

    * Optional
    * Default: **yes**

    This attribute defines if transcoding is enabled as a whole, possible values are ”yes” or ”no”.

    ::

        fetch-buffer-size=...

    * Optional
    * Default: **262144**

    In case you have transcoders that can not handle online content directly (see the accept-url parameter below), it is
    possible to put the transcoder between two FIFOs, in this case Gerbera will fetch the online content. This setting
    defines the buffer size in bytes, that will be used when fetching content from the web. The value must not be less
    than allowed by the curl library (usually 16384 bytes).

    ::

        fetch-buffer-fill-size=...

    * Optional
    * Default: **0 (disabled)**

    This setting allows to prebuffer a certain amount of data before sending it to the transcoder, this should ensure a
    constant data flow in case of slow connections. Usually this setting is not needed, because most transcoders will just
    patiently wait for data and we anyway buffer on the output end. However, we observed that ffmpeg will fail to transcode flv
    files if it encounters buffer underruns - this setting helps to avoid this situation.

**Child tags:**

``mimetype-profile-mappings``
-----------------------------

.. code-block:: xml

    <mimetype-profile-mappings allow-unused="no">

The mime type to profile mappings define which mime type is handled by which profile.

Different mime types can map to the same profile in case that the transcoder in use supports various input formats.
The same mime type can also map to several profiles, in this case multiple resources in the XML will be generated,
allowing the player to decide which one to take.

The mappings under mimetype-profile are defined in the following manner:

    ::

        allow-unused=...

    Suppress errors when loading profiles. Default **no**: Unused mappings are not allowed in config.

``transcode``
-------------

.. code-block:: xml

  <transcode mimetype="audio/x-flac" using="oggflac-pcm"/>

* Optional

In this example we want to transcode our flac audio files (they have the mimetype audio/x-flac) using the ”oggflac-pcm”
profile which is defined below.

    ::

        mimetype=...

    Selects the mime type of the source media that should be transcoded.

    ::

        using=...

    Selects the transcoding profile that will handle the mime type above. Information on how to define transcoding
    profiles can be found below.


``profiles``
------------

.. code-block:: xml

    <profiles allow-unused="no">

This section defines the various transcoding profiles.

    ::

        allow-unused=...

    Suppress errors when loading profiles. Default **no**: Unused profiles are not allowed in config.

    .. code-block:: xml

        <profile name="oggflag-pcm" enabled="yes" type="external">

    * Optional

    Definition of a transcoding profile.

        ::

            name=...

        * Required

        Name of the transcoding profile, this is the name that is specified in the mime type to profile mappings.

        ::

            enabled=...

        * Required

        Enables or disables the profile, allowed values are ”yes” or ”no”.

        ::

            type=...

        * Required

        Defines the profile type, currently only ”external” is supported, this will change in the future.

    .. code-block:: xml

        <mimetype>audio/x-wav</mimetype>

    * Required

    Defines the mime type of the transcoding result (i.e. of the transcoded stream). In the above example we transcode to PCM.

    .. code-block:: xml

        <accept-url>yes</accept-url>

    * Optional
    * Default: **yes**

    Some transcoders are able to handle non local content, i.e. instead giving a local file name you can pass an
    URL to the transcoder. However, some transcoders can only deal with local files, for this case set the value to ”no”.

    .. code-block:: xml

        <first-resource>no</first-resource>

    * Optional
    * Default: **no**

    It is possible to offer more than one resource in the browse result, a good player implementation will go
    through all resources and pick the one that it can handle best. Unfortunately most players only look at the
    first resource and ignore the rest. When you add a transcoding profile for a particular media type it will
    show up as an additional resource in the browse result, using this parameter you can make sure that the
    transcoded resource appears first in the list.

    Note:
        if more than one transcoding profile is applied on one source media type (i.e. you transcode an OGG file
        to MP3 and to PCM), and the first-resource parameter is specified in both profiles, then the
        resource positions are undefined.

    .. code-block:: xml

        <hide-original-resource>no</hide-original-resource>

    * Optional
    * Default: **no***

    This parameter will hide the resource of the original media when sending the browse result to the player,
    this can be useful if your device gets confused by multiple resources and allows you to send only the
    transcoded one.

    .. code-block:: xml

        <accept-ogg-theora>no</accept-org-theora>

    * Optional
    * Default: **no***

    As you may know, OGG is just a container, the content could be Vorbis or Theora while the mime type is
    ”application/ogg”. For transcoding we need to identify if we are dealing with audio or video content,
    specifying yes in this tag in the profile will make sure that only OGG files containing Theora will be processed.

    .. code-block:: xml

        <avi-fourcc-list mode="ignore">

    * Optional
    * Default: **disabled***

    This option allows to specify a particular list of AVI fourcc strings that can be either set to be ignored or
    processed by the profile.

    Note:
        this option has no effect on non AVI content.

        ::

            mode=...

        * Required

        Specifies how the list should be handled by the transcoding engine, possible values are:

        ::

            "disabled"

        The option is completely disabled, fourcc list is not being processed.

        ::

            "process"

        Only the fourcc strings that are listed will be processed by the transcoding profile, AVI files with other
        fourcc strings will be ignored. Setting this is useful if you want to transcode only some specific
        fourcc's and not transcode the rest.

        ::

            "ignore"

        The fourcc strings listed will not be transcoded, all other codecs will be transcoded. Setting this
        might be useful if you want to prevent a limited number of codecs from being transcoded, but want to
        apply transcoding on the rest (i.e. - do not transcode divx and xvid, but want to transcode mjpg and
        whatever else might be in the AVI container).

         The list of fourcc strings is enclosed in the avi-fourcc-list section:

        .. code-block:: xml

            <fourcc>XVID</fourcc>
            <fourcc>DX50</fourcc>

        etc...

    .. code-block:: xml

        <agent command="ogg123" arguments="-d wav -f %out %in"/>

     * Required

    Defines the transcoding agent and the parameters, in the example above we use ogg123 to convert ogg or flac to wav.

        ::

            command=...

        * Required

        Defines the transcoder binary that will be executed by Gerbera upon a transcode request, the binary
        must be in $PATH. It is very important that the transcoder is capable of writing the output to a FIFO,
        some applications, for example ffmpeg, have problems with that. The command line arguments are specified
        separately (see below).

        ::

            arguments=...

        * Required

        Specifies the command line arguments that will be given to the transcoder application upon execution.
        There are two special tokens:

            ::

                %in
                %out

            Those tokens get substituted by the input file name and the output FIFO name before execution.

    .. code-block:: xml

        <buffer size="1048576" chunk-size="131072" fill-size="262144"/>

    * Required

    These settings help you to achieve a smooth playback of transcoded media. The actual values need to be tuned
    and depend on the speed of your system. The general idea is to buffer the data before sending it out to the
    player, it is also possible to delay first playback until the buffer is filled to a certain amount.
    The prefill should give you enough space to overcome some high bitrate scenes in case your system can not
    transcode them in real time.

        ::

            size=...

        * Required

        Size of the buffer in bytes.

        ::

            chunk-size=...

        * Required

        Size of chunks in bytes, that are read by the buffer from the transcoder. Smaller chunks will produce a
        more constant buffer fill ratio, however too small chunks may slow things down.

        ::

            fill-size=...

        * Required

        Initial fill size - number of bytes that have to be in the buffer before the first read (i.e. before
        sending the data to the player for the first time). Set this to 0 (zero) if you want to disable prefilling.

    .. code-block:: xml

        <resolution>320x240</resolution>

    * Optional
    * Default: **not specified**

    Allows you to tell the resolution of the transcoded media to your player. This may be helpful if you want
    to generate thumbnails for your photos, or if your player has the ability to pick video streams in a
    particular resolution. Of course the setting should match the real resolution of the transcoded media.

    .. code-block:: xml

        <sample-frequency>source</sample-frequency>

    * Optional
    * Default: **source**

    Specifies the sample frequency of the transcoded media, this information is passed to the player and is
    particularly important when streaming PCM data. Possible values are:

    * **source** - automatically set the same frequency as the frequency of the source content, which is useful if you are not doing any resampling
    * **off** - do not provide this information to the player
    * **frequency**  - specify a fixed value, where `frequency` is a numeric value > 0

    .. code-block:: xml

        <audio-channels>source</audio-channels>

    * Optional
    * Default: **source**

    Specifies the number of audio channels in the transcoded media, this information is passed to the player and
    is particularly important when streaming PCM data. Possible values are:

    * **source** -  automatically set the same number of audio channels as in the source content
    * **off** - do not provide this information to the player
    * **number** - specify a fixed value, where *number* is a numeric value > 0

    .. code-block:: xml

        <thumbnail>yes</thumbnail>

    * Optional
    * Default: **no**

    Note:
        this is an experimental option, the implementation will be refined in the future releases.

    This is a special option which was added for the PS3 users. If the resolution option (see above) was set, then,
    depending on the resolution an special DLNA tag will be added, marking the resource as a thumbnail.
    This is useful if you have a transcoding script that extracts an image out of the video and presents it as a thumbnail.

    Use the option with caution, no extra checking is being done if the resulting mimetype represents an image,
    also, it is will only work if the output of the profile is a JPG image.
