/*GRB*
  Gerbera - https://gerbera.io/

  metadata.js - this file is part of Gerbera.

  Copyright (C) 2022 Gerbera Contributors

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

const arr = object_script_path.split('.');
print("Processing metafile: " + object_script_path + " for " + obj.location + " " + arr[arr.length-1].toLowerCase());
switch (arr[arr.length-1].toLowerCase()) {
    case "nfo":
        parseNfo(obj, object_script_path);
        updateCdsObject();
        break;
}
