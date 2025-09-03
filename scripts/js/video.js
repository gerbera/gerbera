/*GRB*
  Gerbera - https://gerbera.io/

  video.js - this file is part of Gerbera.

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

// doc-add-video-begin
function importVideo(obj, cont, rootPath, autoscanId, containerType) {
  const video = getVideoDetails(obj, rootPath);
  obj.sortKey = '';
  obj.title = video.title;
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/video'];
  const boxes = [
    BK_videoRoot,
    BK_videoAll,
    BK_videoAllDirectories,
    BK_videoAllYears,
    BK_videoAllDates,
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    video: _Chain[BK_videoRoot],
    allVideo: _Chain[BK_videoAll],
    allDirectories: _Chain[BK_videoAllDirectories],
    allYears: _Chain[BK_videoAllYears],
    allDates: _Chain[BK_videoAllDates],

    year: {
      title: boxSetup[BK_videoUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
    },
    month: {
      title: boxSetup[BK_videoUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    date: {
      title: boxSetup[BK_videoUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };
  chain.video.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_VIDEO_ITEM];
  var container = addContainerTree([chain.video, chain.allVideo]);
  const result = [];

  createUserChain(obj, video, _Chain, boxSetup, chainSetup, result, rootPath);
  result.push(addCdsObject(obj, container, rootPath));

  // Year
  if (boxSetup[BK_videoAll].enabled && video.month.length > 0) {
    chain.year.title = video.year;
    chain.month.title = video.month;
    result.push(addCdsObject(obj, addContainerTree([chain.video, chain.allYears, chain.year, chain.month]), rootPath));
  }

  // Dates
  if (boxSetup[BK_videoAllDates].enabled && video.date.length > 0) {
    chain.date.title = video.date;
    result.push(addCdsObject(obj, addContainerTree([chain.video, chain.allDates, chain.date]), rootPath));
  }

  // Directories
  if (boxSetup[BK_videoAllDirectories].enabled && video.dir.length > 0) {
    var tree = [chain.video, chain.allDirectories];
    for (var i = 0; i < video.dir.length; i++) {
      tree = tree.concat({ title: video.dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER });
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
// doc-add-video-end
