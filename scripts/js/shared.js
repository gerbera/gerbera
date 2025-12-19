/*GRB*
  Gerbera - https://gerbera.io/

  shared.js - this file is part of Gerbera.

  Copyright (C) 2025 Gerbera Contributors

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

// based on https://github.com/gerbera/gerbera/issues/3711#issuecomment-3652136049
function importFsContainers(obj, cont, rootPath, autoscanId, containerType) {
  if (!containerType)
    containerType = UPNP_CLASS_CONTAINER;

  var result = [];

  // Get path segments from helpers in common.js
  var dir = getRootPath(rootPath, obj.location); // returns array of segments

  // Container resource handling (same pattern as other scripts)
  var parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const scriptOptions = config['/import/scripting/virtual-layout/script-options/script-option'];
  var skipFolders = scriptOptions ? scriptOptions['skipFolders'] : '0';
  skipFolders = (skipFolders) ? Number.parseInt(skipFolders) : 0;
  const containerRefID = (cont && cont.res && cont.res.count > 0) ? cont.id : obj.id;

  // Add the item to the final container (leaf) or root if no dirs
  var leafChain = [];
  const dirLength = dir ? dir.length : 0;
  parentCount = dirLength - parentCount - 1;
  for (var k = skipFolders; k < dirLength; k++) {
    leafChain.push({
      title: dir[k],
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: containerType,
      metaData: {},
      res: parentCount <= k ? cont.res : undefined,
      aux: obj.aux,
      refID: containerRefID,
    });
  }

  result.push(addCdsObject(obj, addContainerTree(leafChain), rootPath));

  return result;
}
