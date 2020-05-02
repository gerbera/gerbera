How to import and play CD-Images (CUE-File)
===========================================

Do you want to archive your CDs without loss? With ExactAudioCopy (EAC) you can easily create an image in FLAC format and a CUE file. The CUE file is a text file containing information about the individual tracks. Gerbera now allows you to use scripts to read in the CUE file and play back the individual tracks.

Prerequists
~~~~~~~~~~~

1) You have a image copy of your CD in the FLAC-format and a CUE-File. The FLAC-File may include the cover art.
#) Gerbera with the External-Link-Patch.
#) Linux (here Ubuntu 18.04 is used).
#) A flac player on your system.

::

  sudo apt install flac



Create a transcoder-script
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

  #!/bin/bash
  # flac2wav.sh transcoding script to play a range from a flac file.
  #
  # Usage flac2wav.sh <base64 encoded parameterstring> <outputfile>
  #
  # The <parameterstring> is a base64-encoded URL:
  # <parameterstring decoded> ::= "params?skip=<time>&until=<time>&filename=<filename>
  #   <time> ::= mm:ss.ms
  #   <filename> : Fulpath to the flac-file
  # The <outputfile> is a FIFO-pipe created by gerbera
  #
  INPUT="$1"
  OUTPUT="$2"
  # parse parameters
  PARAMSTR=`echo ${INPUT} | sed 's/http:\/\/cue2flac\/\(.*\).*/\1/' | base64 --decode`
  SKIP=`echo ${PARAMSTR} | sed 's/params?.*skip=\([0-9:.]*\).*/\1/'`  
  UNTIL=`echo ${PARAMSTR} | sed 's/params?.*until=\([0-9:.\-]*\).*/\1/'`
  FILENAME=`echo ${PARAMSTR} | sed 's/params?.*filename=\"\(.*\)\".*/\1/'`
  # default params
  if [ "${SKIP}" == "" ]
  then
    SKIP="0:00.00"
  fi

  if [ "${UNTIL}" == "" ]
  then
    UNTIL="-0:00.00"
  fi

  # if a filename is given, try transcoding
  if [ "${FILENAME}" != "" ]
  then
    # transcoding command from flac to raw wav with a range
    exec /usr/bin/flac -f -c -d "${FILENAME}" --skip=${SKIP} --until=${UNTIL} > ${OUTPUT} 2>/dev/null
  fi


Modify Your Import Scripts
~~~~~~~~~~~~~~~~~~~~~~~~~~

Add following code to the playlist.js script inside the if-statement.

::

  else if (type == 'cue')
  {
    // .cue file parsing tested only with one audio file and multiplse tracks, 
    // but this should work also with multiple audio files
    var thisGenre = null;
    var thisDate = null;
    var discArtist = null;
    var discTitle = null;
    var thisArtist = null;
    var thisTitle = null;
    var thisFileName = null;
    var thisTrackIdx = null;
    var thisStart = "0:00.00";
    var thisStop = "-0:00.00";

    var fileType = null;

    var inTrackIdx = 0;
    var indexDone = 0;

    var oldGenre = null;
    var oldDate = null;
    var oldArtist = null;
    var oldTitle = null;
    var oldFileName = null;
    var oldTrackIdx = null;
    var oldStart = "0:00.00";
    var oldStuff = 0;
    var regarray;

    var inLine = readln();
    do
    {
    if ((regarray = inLine.match(/^REM\s*GENRE\s+(\S+)$/))){
      thisGenre = p2i(regarray[1]);
    } else if ((regarray = inLine.match(/^REM\s*DATE\s+(\d+)$/))){
      thisDate = parseInt(regarray[1], 10);
    } else if ((regarray = inLine.match(/^\s*PERFORMER\s+\"(\S.+)\"$/))){
      if (inTrackIdx < 1){
      // no track specified yet -> assign also the  disc artist
      discArtist = p2i(regarray[1]);
      thisArtist = p2i(regarray[1]);
      } else {
      thisArtist = p2i(regarray[1]);
      }
    } else if ((regarray = inLine.match(/^TITLE\s+\"(\S.+)\"$/))){
      if (inTrackIdx < 1){
      // not in track -> disc title
      discTitle = p2i(regarray[1]);
      } else {
      // inside a track -> track title
      thisTitle = p2i(regarray[1]);
      }
    } else if ((regarray = inLine.match(/^\s*FILE\s+\"(\S.+)\"\s+(\S.+)$/))){
      thisFileName = f2i(regarray[1]);
      fileType = regarray[2];
      if (indexDone > 0){
      // multiple files in same cue -> add entry for this one

      createSubItem(thisFileName, thisGenre, thisDate, discArtist, discTitle, thisArtist, thisTitle, thisTrackIdx, thisStart, thisStop);
      indexDone = 0;
      thisStart = "0:00.00";
      thisStop = "-0:00.00";
      inTrackIdx = 0;
      }
    } else if ((regarray = inLine.match(/^\s*TRACK\s+(\d+)\s*AUDIO\s*$/))){
      thisTrackIdx = parseInt(regarray[1], 10);
      if ((thisTrackIdx != inTrackIdx) && (inTrackIdx > 0)){
       // a new track, so the earlier should be ok
       oldGenre = thisGenre;
       oldDate = thisDate;
       oldArtist = thisArtist;
       oldTitle = thisTitle;
       oldFileName = thisFileName;
       oldTrackIdx = inTrackIdx;
       oldStart = thisStart;
       oldStuff = 1;
      }
      inTrackIdx = thisTrackIdx;
      indexDone = 0;

      inTrack = 1;
    } else if ((regarray = inLine.match(/^\s*INDEX\s+(\d+)\s*(\d+):(\d+):(\d+)\s*$/))){
      var tmpIdx = parseInt(regarray[1], 10);
      var tmpMin = parseInt(regarray[2], 10);
      var tmpSec = parseInt(regarray[3], 10);
      var tmpFrames = parseInt(regarray[4], 10);

      // the time stamp is in min:sec:frames format which has to be changes
      var timeSec = tmpSec + tmpFrames/74.0;

      var tmpStamp = null;
      if (timeSec < 10){
      tmpStamp = tmpMin + ':0' + timeSec;
      } else {
      tmpStamp = tmpMin + ':' + timeSec;
      }

      if (indexDone < 1){
       if (oldStuff > 0){
        createSubItem(oldFileName, oldGenre, oldDate, discArtist, discTitle, oldArtist, oldTitle, oldTrackIdx, oldStart, tmpStamp);
        indexDone = 0;
        thisStart = tmpStamp;
        thisStop = "-0:00";

        oldGenre = null;
        oldDate = null;
        oldArtist = null;
        oldTitle = null;
        oldFileName = null;
        oldTrackIdx = null;
        oldStart = null;
        oldStuff = 0;
       } else {
        indexDone = 1;
       }
      }
    }
    inLine = readln();
     }
     while (inLine);
     createSubItem(thisFileName, thisGenre, thisDate, discArtist, discTitle, thisArtist, thisTitle, thisTrackIdx, thisStart, thisStop);
  }

And add a function createSubItem to playlist.js:

::
 
  function createSubItem(thisFileName, thisGenre, thisDate, thisDiscArtist,
       thisDiscTitle, thisArtist, thisTitle, thisTrackIdx, thisStart, thisStop)
  {
    if (thisFileName && thisFileName.substr(0,1) != '/'){
     thisFileName = playlistLocation + thisFileName;
    }
    var newItem = new Object();
    newItem.objectType = OBJECT_TYPE_ITEM_EXTERNAL_URL;
    // encode the information to a pseudo-url that is parsed in the transcoding script
    newItem.title = thisFileName;
    var param = "params?skip=";
    if (thisStart){
     param += thisStart;
    } else {
     param +=  "0:00.00";
    }
    param +=  "&until=";
    if (thisStop){
     param +=  thisStop;
    } else {
     param +=  "-0:00.00";
    }
    param +=  "&filename=\"" + thisFileName + "\"";
    newItem.parameter = btoa(param);
    newItem.location = playlistLocation;
    newItem.upnpclass = UPNP_CLASS_ITEM_MUSIC_TRACK;
    newItem.theora = 0;
    newItem.onlineservice = ONLINE_SERVICE_NONE;
    newItem.mimetype = "audio/x-cue+flac";
    newItem.meta = new Array();
    if (thisTitle){
    newItem.meta[M_TITLE] = thisTitle;
    } else {
    newItem.meta[M_TITLE] = thisFileName;
    }
    if (thisArtist){
    newItem.meta[M_ARTIST] = thisArtist;
    } else {
    if (thisDiscArtist){
      newItem.meta[M_ARTIST] = thisDiscArtist;
    }
    }
    if (thisDiscTitle){
    newItem.meta[M_ALBUM] = thisDiscTitle;
    }
    if (thisDate){
    newItem.meta[M_DATE] = thisDate;
    }
    if (thisGenre){
     newItem.meta[M_GENRE] = thisGenre;
    }
    if (thisTrackIdx){
     newItem.meta[M_TRACKNUMBER] = thisTrackIdx;
     newItem.playlistOrder = thisTrackIdx;
    }
    addAudio(newItem);
  }

You see that we create for each track a external link object with a mime type "audio/x-cue+flac". We will need "our" mime type to use our transcoding script. 

In my case the JavaScipt-funtion btoa was not included in the runtime environment so we add our own btoa() to common.js.

::

  var Base64 = new function() {
    var keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
    this.encode = function(input) {
    var output = "";
    var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
    var i = 0;
    input = Base64._utf8_encode(input);
    while (i < input.length) {
      chr1 = input.charCodeAt(i++);
      chr2 = input.charCodeAt(i++);
      chr3 = input.charCodeAt(i++);
      enc1 = chr1 >> 2;
      enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
      enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
      enc4 = chr3 & 63;
      if (isNaN(chr2)) {
      enc3 = enc4 = 64;
      } else if (isNaN(chr3)) {
      enc4 = 64;
      }
      output = output + keyStr.charAt(enc1) + keyStr.charAt(enc2) + keyStr.charAt(enc3) + keyStr.charAt(enc4);
    }
    return output;
    }

    this.decode = function(input) {
    var output = "";
    var chr1, chr2, chr3;
    var enc1, enc2, enc3, enc4;
    var i = 0;
    input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");
    while (i < input.length) {
      enc1 = keyStr.indexOf(input.charAt(i++));
      enc2 = keyStr.indexOf(input.charAt(i++));
      enc3 = keyStr.indexOf(input.charAt(i++));
      enc4 = keyStr.indexOf(input.charAt(i++));
      chr1 = (enc1 << 2) | (enc2 >> 4);
      chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
      chr3 = ((enc3 & 3) << 6) | enc4;
      output = output + String.fromCharCode(chr1);
      if (enc3 != 64) {
      output = output + String.fromCharCode(chr2);
      }
      if (enc4 != 64) {
      output = output + String.fromCharCode(chr3);
      }
    }
    output = Base64._utf8_decode(output);
    return output;
    }

    this._utf8_encode = function(string) {
    string = string.replace(/\r\n/g, "\n");
    var utftext = "";
    for (var n = 0; n < string.length; n++) {
      var c = string.charCodeAt(n);
      if (c < 128) {
      utftext += String.fromCharCode(c);
      } else if ((c > 127) && (c < 2048)) {
      utftext += String.fromCharCode((c >> 6) | 192);
      utftext += String.fromCharCode((c & 63) | 128);
      } else {
      utftext += String.fromCharCode((c >> 12) | 224);
      utftext += String.fromCharCode(((c >> 6) & 63) | 128);
      utftext += String.fromCharCode((c & 63) | 128);
      }
    }
    return utftext;
    }

    this._utf8_decode = function(utftext) {
    var string = "";
    var i = 0;
    var c = 0,
      c1 = 0,
      c2 = 0,
      c3 = 0;
    while (i < utftext.length) {
      c = utftext.charCodeAt(i);
      if (c < 128) {
      string += String.fromCharCode(c);
      i++;
      } else if ((c > 191) && (c < 224)) {
      c2 = utftext.charCodeAt(i + 1);
      string += String.fromCharCode(((c & 31) << 6) | (c2 & 63));
      i += 2;
      } else {
      c2 = utftext.charCodeAt(i + 1);
      c3 = utftext.charCodeAt(i + 2);
      string += String.fromCharCode(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
      i += 3;
      }
    }
    return string;
    }
  }()

  var btoa = Base64.encode;
  var atob = Base64.decode;

Inside the function addAudio(obj) insert following code according to your virtual layout:

::

  if (obj.objectType == OBJECT_TYPE_ITEM_EXTERNAL_URL) {
    chain = new Array('CD Images', artist, album);
    // until now obj.location contains the full path of the object
    // the attribute obj.parameter has got the base64 encoded parameter string
    // now we must create the location with a pseudo-url
    obj.location = "http://cue2flac/" + obj.parameter;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
  }


Edit Your config.xml
~~~~~~~~~~~~~~~~~~~~

In the config.xml you must tell gerbera to treat cue-files as a playlist. Make sure that <playlist-script> is set correctly:

::

  <mappings>
    ...
    <extension-mimetype ignore-unknown="no">
      ...
      <map from="cue" to="audio/x-cue"/>   
      ...
    </extension-mimetype>
    ...
    <mimetype-contenttype>
      ...
      <treat mimetype="audio/x-cue" as="playlist"/>
      ...
    </mimetype-contenttype>
    ...
  </mappings>

Enable transcoding and set the correct path to the transcoding script.

::

  <transcoding enabled="yes">
    <mimetype-profile-mappings>
      ...
      <transcode mimetype="audio/x-cue+flac" using="flac2wav"/>
      ...
    </mimetype-profile-mappings>
    <profiles>
      ...
      <profile name="/your/path/to/flac2wav" enabled="yes" type="external">
        <mimetype>audio/x-wav</mimetype>
        <accept-url>yes</accept-url>
        <first-resource>yes</first-resource>
        <hide-original-resource>yes</hide-original-resource>
        <accept-ogg-theora>no</accept-ogg-theora>
        <agent command="flac2wav.sh" arguments="%in %out"/>
        <buffer size="1048576" chunk-size="131072" fill-size="65536"/>
      </profile>
      ...
    </profiles>
  </transcoding>
  
Have fun!
