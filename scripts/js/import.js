/*GRB*
  Gerbera - https://gerbera.io/

  import.js - this file is part of Gerbera.

  Copyright (C) 2018-2023 Gerbera Contributors

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

function importItem(item) {
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
                    addAudioStructured(obj, object_script_path, grb_container_type_audio);
                    break;
                default:
                    addAudio(obj, object_script_path, grb_container_type_audio);
                    break;
                }
                break;
            case "object.item.videoItem":
            case "object.item.videoItem.movie":
                addVideo(obj, object_script_path, grb_container_type_video);
                break;
            case "object.item.videoItem.musicVideoClip":
            case "object.item.videoItem.videoBroadcast":
                addTrailer(obj);
                break;
            case "object.item.imageItem":
            case "object.item.imageItem.photo":
                addImage(obj, object_script_path, grb_container_type_image);
                break;
            case "object.item.textItem":
            case "object.item.bookmarkItem":
            case "object.item.playlistItem":
                print("Unable to handle upnp class " + item.upnpclass + " for " + obj.location);
                break;
            default:
                print("Unable to handle upnp class " + item.upnpclass + " for " + obj.location);
                if (mime === 'video' && obj.onlineservice === ONLINE_SERVICE_APPLE_TRAILERS) {
                    mime = 'trailer';
                } else if (item.mimetype === 'application/ogg') {
                    mime = (item.theora === 1) ? 'video' : 'audio';
                }
                switch (mime) {
                    case "audio":
                        switch (audioLayout) {
                        case 'Structured':
                            addAudioStructured(obj, object_script_path, grb_container_type_audio);
                            break;
                        default:
                            addAudio(obj, object_script_path, grb_container_type_audio);
                            break;
                        }
                        break;
                    case "video":
                        addVideo(obj, object_script_path, grb_container_type_video);
                        break;
                    case "trailer":
                        addTrailer(obj);
                        break;
                    case "image":
                        addImage(obj, object_script_path, grb_container_type_image);
                        break;
                    default:
                        print("Unable to handle mime type " + item.mimetype + " for " + obj.location);
                        break;
                }
                break;
        }
    }
}

function importAudio(obj, rootPath, autoscanId, containerType) {
    addAudio(obj, rootPath, containerType);
}
function importAudioStructured(obj, rootPath, autoscanId, containerType) {
    addAudioStructured(obj, rootPath, containerType);
}
function importVideo(obj, rootPath, autoscanId, containerType) {
    addVideo(obj, rootPath, containerType);
}
function importImage(obj, rootPath, autoscanId, containerType) {
    addImage(obj, rootPath, containerType);
}
function importTrailer(obj, rootPath, autoscanId, containerType) {
    addTrailer(obj);
}

// Global Variables
var orig;
// compatibility with older configurations
if (orig && orig !== undefined)
    importItem(orig);
