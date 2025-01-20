/*GRB*
  Gerbera - https://gerbera.io/

  import.js - this file is part of Gerbera.

  Copyright (C) 2018-2025 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/

function importItem(item, cont) {
  if (getPlaylistType(item.mimetype) === '') {
    var arr = item.mimetype.split('/');
    var mime = arr[0];

    // All virtual objects are references to objects in the
    // PC-Directory, so make sure to correctly set the reference ID!
    var obj = item;
    obj.refID = item.id;

    const upnpClass = item.upnpclass;
    var audioLayout = config['/import/scripting/virtual-layout/attribute::audio-layout'];
    if (!audioLayout) {
      audioLayout = 'Default';
    }
    switch (upnpClass) {
      case "object.item.audioItem":
      case "object.item.audioItem.musicTrack":
      case "object.item.audioItem.audioBook":
      case "object.item.audioItem.audioBroadcast":
        switch (audioLayout) {
          case 'Structured':
            return addAudioStructured(obj, cont, object_script_path, grb_container_type_audio);
            break;
          default:
            return addAudio(obj, cont, object_script_path, grb_container_type_audio);
            break;
        }
        break;
      case "object.item.videoItem":
      case "object.item.videoItem.movie":
        return addVideo(obj, cont, object_script_path, grb_container_type_video);
        break;
      case "object.item.videoItem.musicVideoClip":
      case "object.item.videoItem.videoBroadcast":
        return addTrailer(obj);
        break;
      case "object.item.imageItem":
      case "object.item.imageItem.photo":
        return addImage(obj, cont, object_script_path, grb_container_type_image);
        break;
      case "object.item.textItem":
      case "object.item.bookmarkItem":
      case "object.item.playlistItem":
        print2("Error", "Unable to handle upnp class '" + item.upnpclass + "' for " + obj.location);
        break;
      default:
        print2("Warning", "Unable to handle unknown upnp class '" + item.upnpclass + "' for " + obj.location);
        if (item.mimetype === 'application/ogg') {
          mime = (item.theora === 1) ? 'video' : 'audio';
        }
        switch (mime) {
          case "audio":
            switch (audioLayout) {
              case 'Structured':
                return addAudioStructured(obj, cont, object_script_path, grb_container_type_audio);
                break;
              default:
                return addAudio(obj, cont, object_script_path, grb_container_type_audio);
                break;
            }
            break;
          case "video":
            return addVideo(obj, cont, object_script_path, grb_container_type_video);
            break;
          case "trailer":
            return addTrailer(obj);
            break;
          case "image":
            return addImage(obj, cont, object_script_path, grb_container_type_image);
            break;
          default:
            print2("Error", "Unable to handle mime type '" + item.mimetype + "' for " + obj.location);
            break;
        }
        break;
    }
  }
}

// doc-import-begin
function importAudio(obj, cont, rootPath, autoscanId, containerType) {
  return addAudio(obj, cont, rootPath, containerType);
}
function importAudioStructured(obj, cont, rootPath, autoscanId, containerType) {
  return addAudioStructured(obj, cont, rootPath, containerType);
}
function importVideo(obj, cont, rootPath, autoscanId, containerType) {
  return addVideo(obj, cont, rootPath, containerType);
}
function importImage(obj, cont, rootPath, autoscanId, containerType) {
  return addImage(obj, cont, rootPath, containerType);
}
function importOnlineItem(obj, cont, rootPath, autoscanId, containerType) {
  return addTrailer(obj);
}
// doc-import-end

// Global Variables
var orig;
var cont;
var object_ref_list;
// compatibility with older configurations
if (!cont || cont === undefined)
  cont = orig;
if (orig && orig !== undefined)
  object_ref_list = importItem(orig, cont);
