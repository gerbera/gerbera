/*GRB*
  Gerbera - https://gerbera.io/

  import.js - this file is part of Gerbera.

  Copyright (C) 2018-2021 Gerbera Contributors

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

// main script part

if (getPlaylistType(orig.mimetype) === '') {
    var arr = orig.mimetype.split('/');
    var mime = arr[0];

    // All virtual objects are references to objects in the
    // PC-Directory, so make sure to correctly set the reference ID!
    var obj = orig; 
    obj.refID = orig.id;

    if (mime === 'video' && obj.onlineservice === ONLINE_SERVICE_APPLE_TRAILERS) {
        mime = 'trailer';
    } else if (orig.mimetype === 'application/ogg') {
        mime = (orig.theora === 1) ? 'video' : 'audio';
    }
    switch (mime) {
        case "audio":
            addAudio(obj);
            break;
        case "video":
            addVideo(obj);
            break;
        case "trailer":
            addTrailer(obj);
            break;
        case "image":
            addImage(obj);
            break;
        default:
            print("Unable to handle mime type " + orig.mimetype + " for " + obj.location);
            break;
    }
}
