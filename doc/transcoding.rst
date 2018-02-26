.. index:: Transcoding Content

Transcoding Content
===================

Transcoding allows you to perform format conversion of your content on the fly allowing you to view media that is otherwise
not supported by your player.

For example, you might have your music collection stored in the OGG format, but your player only supports MP3 or you have your
movies stored in DivX format, but your player only supports MPEG2 and MPEG4. Of course you could sit down and convert
everything before viewing, but that is usually a time consuming procedure, besides, you often you want to keep your original
data untouched and end up storing both, the converted and the original content - wasting space on your hard disk. That's
where on the fly transcoding comes into play.

Another use case is online content - it is often presented in flv or asf formats, you may get mms or rtp streams which your
player can not handle. The transcoding feature makes it possible to access such content.

Last but not least - subtitles. Only a few devices provide subtitle support, usually it's a proprietary solution not
covered by UPnP. Using transcoding you can enable subtitles independent of the player device.

Theory of Operation
~~~~~~~~~~~~~~~~~~~

This chapter describes the idea behind the current transcoding implementation.

What Happens on the User Level
------------------------------

So how does this work? First, let's look at the normal situation where you are playing content that is natively
supported by your player, let's say a DivX movie. You add it to the server, browse the content on your device, hit play and
start streaming the content. Content that the player can not handle is usually grayed out in the on screen display or marked
as unsupported.

Now, what happens if transcoding is in place?

First, you define transcoding profiles, specifying which formats should be converted, let's assume that you have some
music stored in the FLAC format, but your device only supports MP3 and WAV. So, you can define that all FLAC media should be
transcoded to WAV. You then start Gerbera and browse the content as usual on your device, if everything was set up
correctly you should see that your FLAC files are marked as playable now. You hit play, just like usual, and you will see
that your device starts playback.

Here is what happens in the background: when you browse Gerbera, we will look at the transcoding profile that you
specified and, assuming the example above, tell your player that each FLAC file is actually a WAV file. Remember, we
assumed that the player is capable of playing WAV content, so it will display the items as playable. As soon as you press
play, we will use the options defined in the transcoding profile to launch the transcoder, we will feed it the original
FLAC file and serve the transcoded WAV output directly to your player. The transcoding is done on the fly, the files are not
stored on disk and do not require additional disk space.

Technical Background
--------------------

The current implementation allows to plug in any application to do the transcoding. The only important thing is, that the
application is capable of writing the output to a FIFO. Additionally, if the application is not capable of accessing
online content directly we can proxy the online data and provide a FIFO for reading.

The application can be any executable and is launched as a process with a set of given parameters that are defined in the
profile configuration. The special command line tokes %in and %out that are used in the profile will be substituted by the
input file name or input URL and the output FIFO name.

So, the parameters tell the transcoding application: read content from this file, transcode it, and write the output to
this FIFO. MediaTomb will read the output from the FIFO and serve the transcoded stream to the player device.

Buffering is implemented to allow smooth playback and compensate for high bitrate scenes that may require more CPU
power in the transcoding process.

Once you press stop or once you reach end of file we will make sure that the transcoding process is killed and we will clean
up the FIFOs.

The chosen approach is extremely flexible and gives you maximum freedom of choice - you can also use this framework view mms
and rtp streams even if this is originally not supported by your player, blend in subtitles or even listen to text
documents using a text to speech processor.

**Note:**
  it is possible and may be more convenient to call a wrapper script and not the transcoding application
  directly, however, in this case make sure that your shell script uses exec when calling the transcoder.
  Otherwise we will not be able to kill it.


Sample Configuration
~~~~~~~~~~~~~~~~~~~~

We will not go through all possible configuration tags here, they are described in detail in the main documentation.
Instead, we will show an sample configuration and describe the creation process.

First of all you need to decide what content has to be transcoded. It makes no sens to transcode something that can be
played natively by your device. Next, you have to figure out how smart your device is - UPnP defines a way in which it is
possible to provide several resources (or several format representations) of the same content, however most devices only
look at the first resource and ignore the rest. We implemented options to overcome this, however it may get tricky if you have
several devices around and if each of them needs different settings.

All settings apply to your ``config.xml``.


Profile Selection
-----------------

What do we want to transcode? Let's assume that you have some .flv files on your drive or that you want to watch YouTube
videos on your device using MediaTomb. I have not yet heard of a UPnP player device that natively supports flash video, so
let's tell MediaTomb what we want to transcode all .flv content to something that our device understands.

This can be done in the mimetype-profile section under transcoding, mappings:

.. code-block:: xml

    <transcode mimetype="video/x-flv" using="vlcprof"/>

So, we told MediaTomb to transcode all video/x-flv content
using the profile named ``vlcprof``.


Profile Definition
------------------

We define ``vlcprof`` in the profiles section:

.. code-block:: xml

    <profile name="vlcprof" enabled="yes" type="external">
        <mimetype>video/mpeg</mimetype>
        <agent command="vlc"
            arguments="-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit"/>
        <buffer size="10485760" chunk-size="131072" fill-size="2621440"/>
        <accept-url>yes</accept-url>
        <first-resource>yes</first-resource>
    </profile>

Let's have a closer look:

.. code-block:: xml

  <profile name="vlcprof" enabled="yes" type="external">

The profile tag defines the name of the profile - in our
example it's ``vlcprof``, it allows you to quickly switch the
profile on and off by setting the enabled parameter to "yes" or
"no" and also defines the profile type. Currently only one
transcoding type is supported - "external".


Specifying The Target Mime Type
-------------------------------

We need to define which mime type we are transcoding to - that's what the player device will see. It must be something it
supports and there are also some other limitations: the output format must be streamable - meaning, it must be a format which
can be played back without the need of seeking in the stream. AVI is a good example - it contains the index at the end of the
file, so the player needs to seek (or use HTTP range requests) to read the index. Because of that you will not be able to
transcode to AVI on the fly. A good target format is MPEG2 - it does not require the player to seek in the stream and it can be
encoded on the fly with reasonable CPU power.

So, let's specify our target mime type:

.. code-block:: xml

    <mimetype>video/mpeg</mimetype>

Bear in mind that this line only tells your player device about the content format, it does not tell anything to the transcoder
application.


Choosing The Transcoder
-----------------------

Now it is time to look at the agent parameter - this tells us which application to execute and it also provides the necessary
command line options for it:

.. code-block:: xml

    <agent command="vlc" arguments="-I dummy %in --sout #transcode{venc=ffmpeg,vcodec=mp2v,vb=4096,fps=25,aenc=ffmpeg,acodec=mpga,ab=192,samplerate=44100,channels=2}:standard{access=file,mux=ps,dst=%out} vlc:quit"/>

In the above example the command to be executed is "vlc, it will be called with parameter specified in the arguments
attribute. Note the special %in and %out tokens - they are not part of the vlc command line but have a special meaning in
MediaTomb. The %in token will be replaced by the input file name (i.e. the file that needs to be transcoded) and the %out
token will be replaced by the output FIFO name, from where the transcoded content will be read by MediaTomb and sent to the
player.

Just to make it clearer:

.. code-block:: xml

    <agent command="executable name" arguments="command line %in %out"/>

So, an agent tag defines the command which is an executable (make sure that it is in $PATH and that you have permissions to
run it), and arguments which are the command line options and where %in and %out tokens are used in the place of the input
and output file names.

**Note:**
  the output format produced by the transcoder must match the target mime type setting.


Buffer Settings
---------------

There are no defaults for the buffer settings, they need to be tuned to the performance of your system and also to the type of
transcoded media if you want to achieve the best result.

The idea behind buffering is the following: let's assume that you are transcoding a high quality video, the source format has
a variable bitrate. Your CPU can handle most scenes in real time, but occasionally some scenes have a higher bitrate which
require more processing power. Without buffering you would not have a fluent playback - you would see stuttering during those
high bitrate scenes. That's where buffering comes into play. Before sending the data to your player for the very first time,
we will delay the start of the playback until the buffer is filled to a certain amount. This should give you enough slack
to overcome those higher bitrate scenes and watch the movie without any stuttering or dropouts. Also, your CPU will not
transcode the stream as fast as it is being played (i.e. real time), but work as fast as it can, filling up the buffer during
lower bitrate scenes and thus giving you the chance to overcome even long scenes with high bitrate.

The buffer accepts three parameters and is defined like this:

.. code-block:: xml

    <buffer size="5242880" chunk-size="102400" fill-size="1048576"/>

Size is the total size of the buffer, fill-size is the amount that has to be filled before sending out data from the buffer
for the first time. Chunk-size is somewhat tricky, as you know we read the transcoded stream from a FIFO, we then put it into
the buffer from where it gets served to the player. We read the data from the transcoder in chunks, once we fill up the chunk
we put it into the buffer, so this setting is defining the size of those chunks. Lower values will make the buffer feel more
responsive (i.e. it will be filled at a more fluent rate), however too low values will decrease performance. Also, do not
set a too high value here since it may prevent smooth playback - data from the buffer is being played out, if you wait for a
too big chunk at the same time you may empty the buffer.


Accepting Or Proxying Online Content
------------------------------------

With MediaTomb it is possible to add items that are not pointing to local content, but to online resources. It can be
an mp3 stream, a YouTube video or some photos stored on the web. In case that the online media is stored in a format that
is not supported by your player, you can use transcoding to convert it. Some transcoding applications, like VLC, handle
online content pretty well, so you can give a URL directly to the transcoder and it will handle the data download itself. You
can even use that to stream mms or rtsp streams, even if they are not directly supported by your player device. Some
transcoders however, can not access online content directly but can only work with local data. For this situation we offer a
special option:

.. code-block:: xml

    <accept-url>no</accept-url>

If this option is set to "no" MediaTomb will handle the download of the content and will feed the input to the
transcoder via a FIFO. Of course the transcoding application must be capable of handling input from a FIFO. This only works
for the HTTP protocol, we do not handle RTSP or MMS streams, use VLC is you want to handle those. When this option is set to
"yes" we will give the URL to the transcoder.


Resource Index
--------------

What is a resource? In this case it's the <res> tag in the XML that is being sent to the player when it browses the server.
Each item can have one or more resources, each resource describes the type of the content by specifying it's mime type
and also tells the player how and where to get the content. So, resources within the item point to same content, but allow to
present it in different formats. In case of transcoding we will offer the original data as well as the transcoded data by using
the resource tags. A well implemented player will look at all resources that are available for the given item and choose the
one that it supports. Unfortunately most players only look at the first resource and ignore the rest, this feature tells us
to place the transcoded resource at the first position so that those renderers will see and take it.

.. code-block:: xml

    <first-resource>yes</first-resource>


Hiding Original Resource
------------------------

Sometimes it may be required that you only present the transcoded resource (read the previous section for explanation
about resources) to the player. This option allows to do so:

.. code-block:: xml

    <hide-original-resource>yes</hide-original-resource>


Advanced Settings
-----------------

Sometimes you encounter a container format but want to transcode it only if it has a specific codec inside. Provided
that MediaTomb was compiled with ffmpeg support we offer fourcc based transcoding settings for AVI files. A sample
configuration for a profile with fourcc specific settings would look like that:

.. code-block:: xml

    <avi-fourcc-list mode="ignore">
        <fourcc>XVID</fourcc>
        <fourcc>DX50</fourcc>
    </avi-fourcc-list>

Please refer to the main documentation on more information regarding the options.

We also provide a way to specify that a profile should only process the Theora codec if an OGG container is encountered:

.. code-block:: xml

    <accept-ogg-theora>yes</accept-ogg-theora>

A new feature that was added in the 0.12 version possibility to specify that transcoded streams should be sent out using
chunked HTTP encoding. This is now the default setting, since chunked encoding is preferred with content where the content
length is not known. The setting can be controlled on a per profile basis using the following parameter:

.. code-block:: xml

    <use-chunked-encoding>yes</use-chunked-encoding>


Testing And Troubleshooting
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The external transcoding feature is very flexible, however there is a price for flexibility: a lot of things can go wrong.
This section will try to cover the most common problems and present some methods on how things can be tested outside of
MediaTomb.


Testing the Transcoder
----------------------

It's a good idea to test your transcoding application before putting together a profile. As described in the previous
sections we get the transcoded stream via a FIFO, so it's important that the transcoder is capable of writing the output
to a FIFO. This can be easily tested in the Linux command prompt.

Open a terminal and issue the following command:
::

    mkfifo /tmp/tr-test

This will create a FIFO called tr-test in the /tmp directory. Open a second terminal, we will use one terminal to run the
transcoder, and another one to examine the output.

For this test we will assume that we want to transcode an OGG file to WAV, the easiest way to do so is to use the ogg123
program which is part of the vorbis-tools package. Running ogg123 with the -d wav -f outfile parameter is exactly what we
want, just remember that our outfile is the FIFO. So, run the following command, replacing some audio file with an OGG file
that is available on your system, in one of the terminals:
::

    ogg123 -d wav -f /tmp/tr-test /some/audio/file.ogg

The program will start and will appear to be hanging - it's blocked because noone is reading from the FIFO. While ogg123 is
hanging, go to the second terminal and try playing directly from the FIFO (in this example we will use VLC to do that):
::

    vlc /tmp/tr-test

If all goes well you should see that ogg123 is coming to life and you should hear the output from VLC - it should play the
transcoded WAV stream.


Troubleshooting
---------------

This section will try to cover the most common problems related to the external transcoding feature.

Media Is Unplayable
-------------------

What if the resulting stream is unplayable?

This can be the case with some media formats and contaeinrs. A good example is the AVI container - it contains the index at
the very end of the file, meaning, that a player needs to seek to the end to get the index before rendering the video. Since
seeking is not possible in transcoded streams you will not be able to transcode something to AVI and watch it from the FIFO.


Transcoding Does Not Start
--------------------------

As explained in the previous sections, transcoding only starts when your player issues an HTTP GET request to the server.
Further, the request must be made to the transcoding URL.

Most common cases are:
 * wrong mime type mapping: are you sure that you specified the source mime type correctly? Recheck the settings in the
   ``<mimetype-profile>`` section. If you are not sure about the source mime type of your media you can always check that
   via the web UI - just pick one of the files in question and click on the Edit icon.

 * wrong output mime type: make sure that the mime type specified in the profile matches the media format that is
   produced by your transcoder.

 * no permissions to execute the transcoding application: check that the user under which MediaTomb is running has
   sufficient permissions to run the transcoding script or application.

 * transcoding script is not executable or is not in ``$PATH``: if you use a wrapper script around your transcoder, make sure
   that it is executable and can be found in ``$PATH`` (unless you specified an absolute name)


Problem Transcoding Online Streams
----------------------------------

Some transcoding applications do not accept online content directly or have problems transcoding online media. If this is
the case, set the ``<accept-url>`` option appropriately (currently MediaTomb only supports proxying of HTTP streams). This will
put the transcoder between two FIFOs, the online content will be downloaded by MediaTomb and fed to the transcoder via a
FIFO.