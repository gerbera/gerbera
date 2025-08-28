/*GRB*
  Gerbera - https://gerbera.io/

  image.js - this file is part of Gerbera.

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

// doc-add-image-begin
function importImage(obj, cont, rootPath, autoscanId, containerType) {
  const image = getImageDetails(obj, rootPath);
  obj.sortKey = '';
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/image'];
  const boxes = [
    BK_imageRoot,
    BK_imageAll,
    BK_imageAllDirectories,
    BK_imageAllYears,
    BK_imageAllDates,
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);

  const chain = {
    imageRoot: _Chain[BK_imageRoot],
    allImages: _Chain[BK_imageAll],
    allDirectories: _Chain[BK_imageAllDirectories],
    allYears: _Chain[BK_imageAllYears],
    allDates: _Chain[BK_imageAllDates],

    year: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
    },
    month: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      efID: containerRefID
    },
    date: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };
  chain.imageRoot.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_IMAGE_ITEM];
  const result = [];

  createUserChain(obj, image, _Chain, boxSetup, chainSetup, result, rootPath);
  result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allImages]), rootPath));

  // Years
  if (boxSetup[BK_imageAllYears].enabled && image.month.length > 0) {
    chain.year.title = image.year;
    chain.month.title = image.month;
    result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allYears, chain.year, chain.month]), rootPath));
  }

  // Dates
  if (boxSetup[BK_imageAllDates].enabled && image.date.length > 0) {
    chain.date.title = image.date;
    result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allDates, chain.date]), rootPath));
  }

  // Directories
  if (boxSetup[BK_imageAllDirectories].enabled && image.dir.length > 0) {
    var tree = [chain.imageRoot, chain.allDirectories];
    for (var i = 0; i < image.dir.length; i++) {
      tree = tree.concat([{ title: image.dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER }]);
    }
    tree[tree.length - 1].upnpclass = containerType;
    tree[tree.length - 1].metaData = {};
    tree[tree.length - 1].res = containerResource;
    tree[tree.length - 1].aux = obj.aux;
    tree[tree.length - 1].refID = containerRefID;
    result.push(addCdsObject(obj, addContainerTree(tree), rootPath));
  }
  return result;
}
// doc-add-image-end
