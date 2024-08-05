/*GRB*
  Gerbera - https://gerbera.io/

  metadata.js - this file is part of Gerbera.

  Copyright (C) 2022-2024 Gerbera Contributors

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

function importMetadata(meta, cont, rootPath, autoscanId, containerType) {
    const arr = rootPath.split('.');
    print2("Info", "Processing metafile: " + rootPath + " for " + meta.location + " " + arr[arr.length-1].toLowerCase());
    var result = [];
    switch (arr[arr.length-1].toLowerCase()) {
        case "nfo":
            parseNfo(meta, rootPath);
            updateCdsObject(meta);
            result.push_back(meta.id);
            break;
    }
    return result;
}

var obj;
var cont;
var object_ref_list;
// compatibility with older configurations
if (!cont || cont === undefined)
    cont = obj;
if (obj && obj !== undefined)
    object_ref_list = importMetadata(obj, cont, object_script_path, -1, "");
