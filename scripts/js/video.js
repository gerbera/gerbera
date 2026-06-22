/*GRB*
  Gerbera - https://gerbera.io/

  video.js - this file is part of Gerbera.

  Copyright (C) 2018-2026 Gerbera Contributors

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
  const result = [];
  createUserChain(obj, video, _Chain, boxSetup, chainSetup, result, rootPath);

  // All Video
  if (boxSetup[BK_videoAll].enabled) {
    var container = addContainerTree([chain.video, chain.allVideo]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  // Year
  if (boxSetup[BK_videoAllYears].enabled && video.month.length > 0) {
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
  getDirectories([chain.video, chain.allDirectories], boxSetup[BK_videoAllDirectories], video.dir, result, rootPath, obj, containerType, containerResource, containerRefID);

  return result;
}
// doc-add-video-end

function importVideoDetail(obj, cont, rootPath, autoscanId, containerType) {
  const containerResource = cont.res;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/video'];
  const boxes = [
    BK_videoRoot,
    BK_videoAll,
    BK_videoAllDirectories,
    BK_videoAllYears,
    BK_videoAllDates,
    BK_topicRoot,
    BK_topic,
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    video: _Chain[BK_videoRoot],
    allVideo: _Chain[BK_videoAll],
    allDirectories: _Chain[BK_videoAllDirectories],
    topicRoot: _Chain[BK_topicRoot],
    topic: _Chain[BK_topic],

    date: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_ITEM_IMAGE,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: cont.id
    },
    subTopic: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: false,
      upnpclass: UPNP_CLASS_CONTAINER_ITEM_IMAGE,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: cont.id
    },
  };
  const result = [];
  const video = getVideoDetails(obj, rootPath);
  obj.title = video.title;
  obj.sortKey = '';
  createUserChain(obj, video, _Chain, boxSetup, chainSetup, result, rootPath);

  if (boxSetup[BK_videoAll].enabled) {
    chain.video.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_VIDEO_ITEM];
    result.push(addCdsObject(obj, addContainerTree([chain.video, chain.allVideo]), rootPath));
  }
  getDirectories([chain.video, chain.allDirectories], boxSetup[BK_videoAllDirectories], video.dir, result, rootPath, obj, containerType, containerResource, cont.id);
  var titlePrefix = '';

  if (video.videoDate) {
    titlePrefix = toDigits(video.videoDate.getHours()) + '-' + toDigits(video.videoDate.getMinutes()) + '-' + toDigits(video.videoDate.getSeconds());
    titlePrefix = toDigits(video.videoDate.getFullYear(), 4) + '-' + toDigits(video.videoDate.getMonth() + 1) + '-' + toDigits(video.videoDate.getDate()) + ((titlePrefix !== '') ? "-" + titlePrefix : "");
    obj.title = (video.model) ? (titlePrefix + "-" + video.model + "-" + video.title) : (titlePrefix + "-" + video.title);
  }

  if (boxSetup[BK_topic].enabled) {
    if (video.subTopic) {
      chain.subTopic.title = video.subTopic;
      if (video.topic)
        chain.topic.title = video.topic;

      if (video.week)
        chain.date.title = video.week;
      else if (video.videoDate)
        chain.date.title = toDigits(video.videoDate.getFullYear(), 4) + "-" + toDigits(video.videoDate.getMonth() + 1);
      result.push(addCdsObject(obj, addContainerTree([chain.topicRoot, chain.topic, chain.subTopic, chain.date]), rootPath));
    }
  }
  return result;
}
