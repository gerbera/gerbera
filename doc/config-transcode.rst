.. index:: Configure Transcoding

#####################
Configure Transcoding
#####################

The transcoding section allows to define ways on how to transcode content.

.. contents::
   :backlinks: entry
.. sectnum::
   :start: 4

***********
Transcoding
***********

.. confval:: transcoding
   :type: :confval:`Section`
   :required: false
..

.. code-block:: xml

   <transcoding enabled="yes" fetch-buffer-size="262144" fetch-buffer-fill-size="0">

This tag defines the transcoding section.

Transcoding Attributes
======================

.. confval:: transcoding enabled
   :type: :confval:`Boolean`
   :required: false
   :default: ``yes``
..

   .. code:: xml

       enabled="no"

This attribute defines if transcoding is enabled as a whole.

.. confval:: fetch-buffer-size
   :type: :confval:`Integer`
   :required: false
   :default: ``262144``
..

   .. code:: xml

       fetch-buffer-size="1048576"

In case you have transcoders that can not handle online content directly (see the accept-url parameter below), it is
possible to put the transcoder between two FIFOs, in this case Gerbera will fetch the online content. This setting
defines the buffer size in bytes, that will be used when fetching content from the web. The value must not be less
than allowed by the curl library (usually 16384 bytes).

.. confval:: fetch-buffer-fill-size
   :type: :confval:`Integer`
   :required: false
   :default: ``0`` `(disabled)`
..

   .. code:: xml

       fetch-buffer-fill-size="262144"

This setting allows to prebuffer a certain amount of data before sending it to the transcoder, this should ensure a
constant data flow in case of slow connections. Usually this setting is not needed, because most transcoders will just
patiently wait for data and we anyway buffer on the output end. However, we observed that ffmpeg will fail to transcode flv
files if it encounters buffer underruns - this setting helps to avoid this situation.

Mimetype Profile Mappings
=========================

.. confval:: mimetype-profile-mappings
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <mimetype-profile-mappings allow-unused="no">
         <transcode mimetype="audio/x-flac" using="oggflac-pcm"/>
      </mimetype-profile-mappings>

The mime type to profile mappings define which mime type is handled by which profile.

Different mime types can map to the same profile in case that the transcoder in use supports various input formats.
The same mime type can also map to several profiles, in this case multiple resources in the XML will be generated,
allowing the player to decide which one to take.

   .. confval:: mapping allow-unused
      :type: :confval:`Boolean`
      :required: false
      :default: ``no``
   ..

      .. code:: xml

          allow-unused="yes"

Suppress errors when loading profiles. Mappings pointing to missing transcoding profiles are ignored
as well as unknown mimetypes.

Transcode
---------

The mappings under mimetype-profile are defined in the following manner:

.. confval:: transcode
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <transcode mimetype="audio/x-flac" using="oggflac-pcm"/>

In this example we want to transcode our flac audio files (they have the mimetype audio/x-flac) using the ”oggflac-pcm”
profile which is defined below.

   .. confval:: transcode mimetype
      :type: :confval:`String`
      :required: true
   ..
   .. confval:: no-transcoding
      :type: :confval:`String`
      :required: false
      :default: `emtpy`
   ..
      .. versionadded:: 2.6.0
      .. code:: xml

         mimetype="video/*" no-transcoding="video/mpeg,video/mp4"

   Selects the mimetype of the source media that should be transcoded.

   Wildcards like ``video/*`` can be used to match all sub types. In that
   case the attribute ``no-transcoding`` contains a comma separated list of
   mime types that should be excluded. This is intended for devices that only
   support a very limited number of media formats.

   .. confval:: source-profile
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code-block:: xml

         source-profile="AVC_MKV_HP_HD_AC3"

   Defines the DLNA profile string of the transcoding source. If set, only files with the DLNA profile are transcoded.
   See :ref:`Import section <contenttype-dlnaprofile>` how to determine the profile.

   .. confval:: client-flags
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code:: xml

         client-flags="TRANSCODE1"

   If the flags match the ones defined in :ref:`Supported Devices <supported-devices>`, the profile is selected for that client.
   Choose ``TRANSCODE1``, ``TRANSCODE2``, ``TRANSCODE3`` or an unused flag, e.g. "0x100000", to avoid collisions with other features.

   .. confval:: using
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code:: xml

         using="prfle"

   Selects the transcoding profile that will handle the mime type above. Information on how to define transcoding
   profiles can be found below.


Profiles
========

.. confval:: profiles
   :type: :confval:`Section`
   :required: false
..

.. code-block:: xml

    <profiles allow-unused="no">

This section defines the various transcoding profiles.

.. confval:: profiles allow-unused
   :type: :confval:`Boolean`
   :required: false
   :default: ``no``
..

   .. code:: xml

       allow-unused="yes"

Suppress errors when loading profiles. If ``no`` Unused profiles are not allowed in config and gerbera refuses to start.

Profile
-------

.. confval:: profile
   :type: :confval:`Section`
   :required: false
..

   .. code-block:: xml

      <profile name="vlcmpeg" enabled="no" type="external" no-transcoding="" dlna-profile="MP4">
        <mimetype>video/mpeg</mimetype>
        <accept-url>yes</accept-url>
        <first-resource>yes</first-resource>
        <accept-ogg-theora>yes</accept-ogg-theora>
        <sample-frequency>-1</sample-frequency>
        <audio-channels>-1</audio-channels>
        <thumbnail>no</thumbnail>
        <hide-original-resource>no</hide-original-resource>
        <avi-fourcc-list mode="Ignore">
          <fourcc>XVID</fourcc>
          <fourcc>DX50</fourcc>
        </avi-fourcc-list>
        <agent command="vlc" 
               arguments="-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc://quit" />
        <buffer size="14400000" chunk-size="512000" fill-size="120000" />
      </profile>

Definition of a transcoding profile.

Profile Attributes
^^^^^^^^^^^^^^^^^^

   .. confval:: profile name
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          name="prfle"

   Name of the transcoding profile, this is the name that is specified in the mime type to profile mappings.

   .. confval:: profile enabled
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          enabled="yes"

   Enables or disables the profile.

   .. confval:: profile client-flags
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code:: xml

          client-flags="TRANSCODE1"

   If the flags match the ones defined in clients, the profile is selected for that client.
   There are are ``TRANSCODE1``, ``TRANSCODE2``, ``TRANSCODE3`` or choose an unused flag,
   e.g. ``0x1000000``, to avoid collisions with other features.

   .. confval:: profile type
      :type: :confval:`Enum` (``external``)
      :required: true
   ..

     .. code:: xml

        type="external"

   Defines the profile type, currently only ``external`` is supported, this will change in the future.

Profile Items
^^^^^^^^^^^^^

   .. confval:: profile mimetype
      :type: :confval:`Enum` (``external``)
      :required: true
   ..

      .. versionchanged:: 2.3.0
      .. code-block:: xml

          <mimetype>audio/x-wav</mimetype>
          <mimetype value="audio/L16">
            <mime-property key="rate" resource="sampleFrequency"/>
            <mime-property key="channels" resource="nrAudioChannels"/>
            <mime-property key="date" metadata="M_DATE"/>
          </mimetype>

   Defines the mimetype of the transcoding result (i.e. of the transcoded stream). In the above example we transcode to PCM.
   There are two variation of this tag. The simple version accepts the target mime type in the content.
   The second version allows setting additional properties which will be appended to the mimetype if the
   respective property value is not empty. The above example will produce, e.g. ``audio/L16;rate=16000;channels=2;date=2024-07-22``.

      .. confval:: profile mimetype key
         :type: :confval:`String`
         :required: true
      ..
         .. code:: xml

             key="rate"

      Key printed in front of the property value.

      .. confval:: profile mimetype resource
         :type: :confval:`String`
         :required: false
         :default: `empty`
      ..

         .. code:: xml

             resource="sampleFrequency"

      Name of a resource attribute to read. The attribute name must match one of the resource attributes and
      must be available on the content resource. See :ref:`Supported Properties <supported-properties>`.

      .. confval:: profile mimetype metadata
         :type: :confval:`String`
         :required: false
         :default: `empty`
      ..

         .. code:: xml

             metadata="M_DATE"

      Name of a resource attribute to read. The attribute name must match on of the metadata fields of the object.
      See :ref:`Supported Properties <supported-properties>`.

   .. confval:: profile dlna-profile
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code-block:: xml

          <dlna-profile>AVC_MKV_HP_HD_AC3</dlna-profile>

   Defines the DLNA profile string of the transcoding result (i.e. of the transcoded stream). If empty, it is determined from the mime type.

   .. confval:: accept-url
      :type: :confval:`Boolean`
      :required: false
      :default: ``yes``
   ..

      .. code-block:: xml

          <accept-url>yes</accept-url>

   Some transcoders are able to handle non local content, i.e. instead giving a local file name you can pass an
   URL to the transcoder. However, some transcoders can only deal with local files, for this case set the value to ``no``.

   .. confval:: first-resource
      :type: :confval:`Boolean`
      :required: false
      :default: ``no``
   ..

      .. code-block:: xml

          <first-resource>yes</first-resource>

   It is possible to offer more than one resource in the browse result, a good player implementation will go
   through all resources and pick the one that it can handle best. Unfortunately most players only look at the
   first resource and ignore the rest. When you add a transcoding profile for a particular media type it will
   show up as an additional resource in the browse result, using this parameter you can make sure that the
   transcoded resource appears first in the list.

   Note:
       if more than one transcoding profile is applied on one source media type (i.e. you transcode an OGG file
       to MP3 and to PCM), and the first-resource parameter is specified in both profiles, then the
       resource positions are undefined.

   .. confval:: hide-original-resource
      :type: :confval:`Boolean`
      :required: false
      :default: ``no``
   ..

      .. code-block:: xml

          <hide-original-resource>yes</hide-original-resource>

   This parameter will hide the resource of the original media when sending the browse result to the player,
   this can be useful if your device gets confused by multiple resources and allows you to send only the
   transcoded one.

   .. confval:: accept-ogg-theora
      :type: :confval:`Boolean`
      :required: false
      :default: ``no``
   ..

      .. code-block:: xml

          <accept-ogg-theora>yes</accept-org-theora>

   As you may know, OGG is just a container, the content could be Vorbis or Theora while the mimetype is
   ``application/ogg``. For transcoding we need to identify if we are dealing with audio or video content,
   specifying yes in this tag in the profile will make sure that only OGG files containing Theora will be processed.

   .. confval:: avi-fourcc-list
      :type: :confval:`Section``
      :required: false
   ..

      .. code-block:: xml

          <avi-fourcc-list mode="ignore">

   This option allows to specify a particular list of AVI fourcc strings that can be either set to be ignored or
   processed by the profile.

   Note:
       This option has no effect on non AVI content.

      .. confval:: avi-fourcc-list mode
         :type: :confval:`Enum` (``disabled|process|ignore``)
         :required: false
         :default: ``disabled``
      ..

      .. code:: xml

          mode="ignore"

      Specifies how the list should be handled by the transcoding engine, possible values are:

      - ``disabled``: The option is completely disabled, fourcc list is not being processed.

      - ``process``: Only the fourcc strings that are listed will be processed by the transcoding profile,
        AVI files with other fourcc strings will be ignored. Setting this is useful if you want to transcode
        only some specific fourcc's and not transcode the rest.

      - ``ignore``: The fourcc strings listed will not be transcoded, all other codecs will be transcoded.
        Setting this might be useful if you want to prevent a limited number of codecs from being transcoded,
        but want to apply transcoding on the rest (i.e. - do not transcode divx and xvid, but want to transcode
        mjpg and whatever else might be in the AVI container).

      The list of fourcc strings is enclosed in the avi-fourcc-list section:

      .. confval:: 4cc fourcc
         :type: :confval:`String`
         :required: false
         :default: `empty`
      ..

         .. code-block:: xml

             <fourcc>XVID</fourcc>
             <fourcc>DX50</fourcc>


   .. confval:: resolution
      :type: :confval:`String`
      :required: false
      :default: `empty`
   ..

      .. code-block:: xml

          <resolution>320x240</resolution>

   Allows you to tell the resolution of the transcoded media to your player. This may be helpful if you want
   to generate thumbnails for your photos, or if your player has the ability to pick video streams in a
   particular resolution. Of course the setting should match the real resolution of the transcoded media.

   .. confval:: sample-frequency
      :type: :confval:`Enum` (``source|off|<frequency>``)
      :required: false
      :default: ``source``
   ..

      .. code-block:: xml

          <sample-frequency>off</sample-frequency>

   Specifies the sample frequency of the transcoded media, this information is passed to the player and is
   particularly important when streaming PCM data. Possible values are:

   - ``source``: automatically set the same frequency as the frequency of the source content, which is useful if you are not doing any resampling
   - ``off``: do not provide this information to the player
   - ``<frequency>`` specify a fixed value, where ``<frequency>`` is a numeric value > 0

   .. confval:: audio-channels
      :type: :confval:`Enum` (``source|off|<number>``)
      :required: false
      :default: ``source``
   ..

      .. code-block:: xml

          <audio-channels>source</audio-channels>

   Specifies the number of audio channels in the transcoded media, this information is passed to the player and
   is particularly important when streaming PCM data. Possible values are:

   - ``source``: automatically set the same number of audio channels as in the source content
   - ``off``: do not provide this information to the player
   - ``<number>``: specify a fixed value, where ``<number>`` is a numeric value > 0

   .. confval:: thumbnail
      :type: :confval:`Boolean``
      :required: false
      :default: ``no``
   ..

      .. code-block:: xml

          <thumbnail>yes</thumbnail>

   Note:
       this is an experimental option, the implementation will be refined in the future releases.

   This is a special option which was added for the PS3 users. If the :confval:`resolution` option was set, then,
   depending on the resolution, a special DLNA tag will be added, marking the resource as a thumbnail.
   This is useful if you have a transcoding script that extracts an image out of the video and presents it as a thumbnail.

   Use the option with caution, no extra checking is being done if the resulting mimetype represents an image,
   also, it is will only work if the output of the profile is a JPG image.


Profile Agent
-------------

.. confval:: agent
   :type: :confval:`Section``
   :required: true
..

   .. code-block:: xml

       <agent command="ogg123" arguments="-d wav -f %out %in"/>
       <agent command="vlc" arguments="-I dummy %in --sout #transcode{...}:standard{...} vlc:quit">
           <environ name="LC_ALL" value="C"/>
       </agent>
   ..

Defines the transcoding agent and the parameters, in the example above we use ogg123 to convert ogg or flac to wav.

   .. confval:: command
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          command="vlc"

   Defines the transcoder binary that will be executed by Gerbera upon a transcode request, the binary
   must be in $PATH. It is very important that the transcoder is capable of writing the output to a FIFO,
   some applications, for example ffmpeg, have problems with that. The command line arguments are specified
   separately (see below).

   .. confval:: arguments
      :type: :confval:`String`
      :required: true
   ..

      .. code:: xml

          arguments="-I dummy %in --sout #transcode{...}:standard{...} vlc:quit"

   Specifies the command line arguments that will be given to the transcoder application upon execution.
   There are two special tokens: ``%in`` and ``%out``. Those tokens get substituted by the input file name 
   and the output FIFO name before execution.

.. confval:: environ
   :type: :confval:`Section`
   :required: false
..

   .. code:: xml

       <environ name="..." value=".."/>

Sets environment variable which may be required by the transcoding process.
Used to overwrite the environment of the gerbera process. The entry can appear multiple times.

   .. confval:: environ name
      :type: :confval:`String`
      :required: true
   ..

   Set name of environment variable.

   .. confval:: environ value
      :type: :confval:`String`
      :required: true
   ..

Profile Buffer
--------------

   Set value of environment variable.

.. confval:: buffer
   :type: :confval:`Section`
   :required: true
..

   .. code-block:: xml

       <buffer size="1048576" chunk-size="131072" fill-size="262144"/>

These settings help you to achieve a smooth playback of transcoded media. The actual values need to be tuned
and depend on the speed of your system. The general idea is to buffer the data before sending it out to the
player, it is also possible to delay first playback until the buffer is filled to a certain amount.
The prefill should give you enough space to overcome some high bitrate scenes in case your system can not
transcode them in real time.

   .. confval:: buffer size
      :type: :confval:`Integer`
      :required: true
   ..

   .. code:: xml

       size="262144"

   Size of the buffer in bytes.

   .. confval:: buffer chunk-size
      :type: :confval:`Integer`
      :required: true
   ..

      .. code:: xml

          chunk-size="65536"

   Size of chunks in bytes, that are read by the buffer from the transcoder. Smaller chunks will produce a
   more constant buffer fill ratio, however too small chunks may slow things down.

   .. confval:: buffer fill-size
      :type: :confval:`Integer`
      :required: true
   ..

      .. code:: xml

          fill-size="65536"

   Initial fill size - number of bytes that have to be in the buffer before the first read (i.e. before
   sending the data to the player for the first time). Set this to ``0`` (zero) if you want to disable prefilling.
