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
      refID: containerRefID
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

function importImageDetail(obj, cont, rootPath, autoscanId, containerType) {
  const containerResource = cont.res;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/video'];
  const scriptOptions = config['/import/scripting/virtual-layout/script-options/script-option'];
  const rawImageFilter = scriptOptions && scriptOptions['rawImageFilter'] ? scriptOptions['rawImageFilter'] : 'image/.*raw';
  const boxes = [
    BK_imageRoot,
    BK_imageAll,
    BK_imageAllDirectories,
    BK_imageAllYears,
    BK_imageAllDates,
    BK_imageAllModels,
    BK_imageYearMonth,
    BK_imageYearDate,
    BK_topicRoot,
    BK_topic,
    BK_topicExtra,
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    imageRoot: _Chain[BK_imageRoot],
    allImages: _Chain[BK_imageAll],
    allDirectories: _Chain[BK_imageAllDirectories],
    allYears: _Chain[BK_imageAllYears],
    allDates: _Chain[BK_imageAllDates],
    allModels: _Chain[BK_imageAllModels],
    yearMonth: _Chain[BK_imageYearMonth],
    yearDate: _Chain[BK_imageYearDate],
    topicRoot: _Chain[BK_topicRoot],
    topic: _Chain[BK_topic],
    topicExtra: _Chain[BK_topicExtra],

    year: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: false,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    month: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_ITEM_IMAGE,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: cont.id
    },
    date: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_ITEM_IMAGE,
      metaData: [],
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
    model: {
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

  chain.imageRoot.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_IMAGE_ITEM];
  chain.topicExtra.res = containerResource;
  chain.topicExtra.aux = obj.aux;
  chain.topicExtra.refID = cont.id;

  var re = new RegExp(rawImageFilter, 'i');
  if (re.exec(obj.mimetype)) // do not handle raw images here
    return [];

  const result = [];
  const image = getImageDetails(obj, rootPath);
  obj.sortKey = '';
  createUserChain(obj, image, _Chain, boxSetup, chainSetup, result, rootPath);
  if (boxSetup[BK_imageAll].enabled) {
    result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allImages]), rootPath));
  }

  if (boxSetup[BK_imageAllDirectories].enabled) {
    var tree = [chain.imageRoot];
    tree = tree.concat(chain.allDirectories);

    const path = image.dir;

    for (var i = 0; i < path.length; i++) {
      if (path[i]) {
        tree = tree.concat({ title: path[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_ITEM_IMAGE });
      }
    }
    tree[tree.length - 1].upnpclass = containerType;
    tree[tree.length - 1].metaData = [];
    tree[tree.length - 1].res = containerResource;
    tree[tree.length - 1].aux = obj.aux;
    tree[tree.length - 1].refID = cont.id;
    result.push(addCdsObject(obj, addContainerTree(tree), rootPath));
  }

  if (image.topic) {
    chain.topic.title = image.topic;
  }

  if (image.subject !== '') {
    chain.topicExtra.title = chain.topic.title + ' ' + chain.topicExtra.title;
  }

  var titlePrefix = '';
  var mTitle = image.title;

  if (image.time) {
    titlePrefix = toDigits(image.imageDate.getHours()) + '-' + toDigits(image.imageDate.getMinutes()) + '-' + toDigits(image.imageDate.getSeconds());
  }
  if (image.date) {
    titlePrefix = toDigits(image.imageDate.getFullYear(), 4) + '-' + toDigits(image.imageDate.getMonth() + 1) + '-' + toDigits(image.imageDate.getDate()) + ((titlePrefix !== '') ? "-" + titlePrefix : "");
  }
  if (image.date || image.time) {
    obj.title = (image.model) ? (titlePrefix + "-" + image.model + "-" + image.title) : (titlePrefix + "-" + image.title);

    chain.year.title = toDigits(image.imageDate.getFullYear(), 4);

    if (image.month) {
      if (boxSetup[BK_imageYearMonth].enabled) {
        chain.month.title = toDigits(image.imageDate.getMonth() + 1);
        result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.yearMonth, chain.year, chain.month]), rootPath));
      }
      if (image.subTopic) {
        chain.subTopic.title = image.subTopic;
        if (image.week !== '')
          chain.date.title = image.week;
        else
          chain.date.title = chain.year.title + "-" + chain.month.title;

        if (boxSetup[BK_topic].enabled) {
          result.push(addCdsObject(obj, addContainerTree([chain.topicRoot, chain.topic, chain.subTopic, chain.date]), rootPath));
        }
        if (image.subject !== '' && boxSetup[BK_topicExtra].enabled) {
          result.push(addCdsObject(obj, addContainerTree([chain.topicRoot, chain.topicExtra, chain.subTopic, chain.date]), rootPath));
        }
      }
    }

    if (boxSetup[BK_imageYearDate].enabled) {
      chain.date.title = image.date;
      result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.yearDate, chain.year, chain.date]), rootPath));
    }
  }
  if (image.time) {
    mTitle = titlePrefix + "-" + mTitle;
  }

  if (boxSetup[BK_imageAllModels].enabled) {
    if (image.model) {
      chain.model.title = image.model;
    }
    obj.title = mTitle;
    result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allModels, chain.model, chain.year, chain.month]), rootPath));
  }

  return result;
}
