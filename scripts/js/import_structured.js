/*GRB*
  Gerbera - https://gerbera.io/

  import_structured.js - this file is part of Gerbera.

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
    
    if (mime === 'audio') {
        addAudioStructured(obj);
    }
    
    if (mime === 'video') {
        if (obj.onlineservice === ONLINE_SERVICE_APPLE_TRAILERS) {
            addTrailer(obj);
        } else {
            addVideo(obj);
        }
    }
    
    if (mime === 'image') {
        addImage(obj);
    }

    if (orig.mimetype === 'application/ogg') {
        if (orig.theora === 1) {
            addVideo(obj);
        } else {
            addAudioStructured(obj);
        }
    }
}
