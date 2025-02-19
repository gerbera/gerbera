/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-tree.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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
import { Autoscan } from "./gerbera-autoscan.module.js";
import { Auth } from "./gerbera-auth.module.js";
import { GerberaApp } from "./gerbera-app.module.js";
import { Trail } from "./gerbera-trail.module.js";
import { Tweaks } from "./gerbera-tweak.module.js";
import { Items } from "./gerbera-items.module.js";
import { Updates } from "./gerbera-updates.module.js";

const treeViewCss = {
  titleClass: 'folder-title',
  closedIcon: 'fa fa-folder',
  openIcon: 'fa fa-folder-open'
};

let currentTree = [];
let currentType = 'db';
let currentView = undefined;
let pendingItems = [];

const destroy = () => {
  currentTree = [];
  currentType = 'db';
  currentView = undefined;
  const tree = $('#tree');
  if (tree.hasClass('grb-tree')) {
    tree.tree('destroy');
  }
  return Promise.resolve();
};

class TreeView {
  parentId = -1;
  constructor(pId) {
    this.parentId = pId;
  }
  get requestData() {
    let result = {
      parent_id: this.parentId,
      select_it: 0
    };
    result[Auth.SID] = Auth.getSessionId();
    return result;
  }
  checkForUpdates() {
    return Promise.resolve();
  }
  hasClass() { return true; }
}

class BrowseTreeView extends TreeView {
  constructor(pId) {
    super(pId);
  }
  get requestData() {
    let result = super.requestData;
    result.req_type = 'containers';
    result.action = 'browse';
    return result;
  }
  checkForUpdates() {
    return Updates.getUpdates(true);
  }
}

class SearchTreeView extends TreeView {
  constructor(pId) {
    super(pId);
  }
  get requestData() {
    let result = super.requestData;
    result.req_type = 'containers';
    result.action = 'search';
    return result;
  }
  hasClass(cls) { return (cls === undefined || !cls.endsWith('dynamicFolder')); }
}

class FileTreeView extends TreeView {
  constructor(pId) {
    super(pId);
  }
  get requestData() {
    let result = super.requestData;
    result.req_type = 'directories';
    result.action = 'browse';
    return result;
  }
}

const initialize = () => {
  $('#tree').html('');
  return Promise.resolve();
};

const viewFactory = (type, parentId) => {
  currentType = type;
  switch (currentType) {
    case 'db':
      currentView = new BrowseTreeView(parentId);
      break;
    case 'search':
      currentView = new SearchTreeView(parentId);
      break;
    default:
      currentView = new FileTreeView(parentId);
      break;
  }
  return currentView;
};

const selectType = (type, parentId) => {
  viewFactory(type, parentId);
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: currentView.requestData
  })
    .then((response) => loadTree(response))
    .then(() => currentView.checkForUpdates())
    .catch((err) => GerberaApp.error(err))
};

const onItemSelected = (event) => {
  const folderList = event.target.parentElement;
  const item = event.data;

  GerberaApp.setCurrentItem(event);

  selectTreeItem(folderList);
  Items.treeItemSelected(item);
};

const onItemExpanded = (event) => {
  const tree = $('#tree');
  const folderName = event.target;
  const folderList = event.target.parentElement;
  const item = event.data;

  GerberaApp.setCurrentItem(event);

  if (item.gerbera && item.gerbera.id) {
    if (tree.tree('closed', folderName) && item.gerbera.childCount > 0) {
      fetchChildren(item.gerbera)
        .then(function (response) {
          if (response) {
            var childTree = transformContainers(response, false);
            tree.tree('append', $(folderList), childTree);
            initSelection(pendingItems);
          }
        })
        .catch((err) => GerberaApp.error(err));
    } else {
      tree.tree('collapse', $(folderList));
    }
  }
};

const onAutoscanEdit = (event) => {
  Autoscan.addAutoscan(event);
};

const onTweakEdit = (event) => {
  Tweaks.addDirTweak(event);
};

const selectTreeItem = (folderList) => {
  const tree = $('#tree');
  tree.tree('select', folderList);
};

const loadTree = (response, config) => {
  const tree = $('#tree');
  const treeViewEvents = {
    onSelection: onItemSelected,
    onExpand: onItemExpanded,
    onAutoscanEdit: onAutoscanEdit,
    onTweakEdit: onTweakEdit,
  };

  config = $.extend({}, treeViewCss, treeViewEvents, config);
  if (response.success) {
    if (tree.hasClass('grb-tree')) {
      tree.tree('destroy');
    }

    Trail.destroy();

    currentTree = transformContainers(response);
    tree.tree({
      data: currentTree,
      config: config
    });

    Trail.makeTrail($('#tree span.folder-title').first());

    pendingItems = GerberaApp.currentItem();

    if (pendingItems) {
      initSelection(pendingItems);
    }
  }
};

const initSelection = (currentItems) => {
  pendingItems = [];
  var success = false;
  currentItems.forEach((item) => {
    if (item.startsWith('grb-tree')) {
      var jq = $('#' + item);
      if (jq && jq.length > 0 && jq[0].children && jq[0].children.length > 1 && jq[0].children[1]) {
        jq[0].children[1].click();
        success = true;
      } else {
        pendingItems.push(item);
      }
    }
  });
  if (!success) { // avoid loop if no item was in tree (anymore)
    pendingItems = [];
  }
};

const fetchChildren = (gerberaData) => {
  currentType = GerberaApp.getType();
  currentView.parentId = gerberaData.id;
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: currentView.requestData
  })
    .catch((err) => {
      GerberaApp.error(err);
    });
};

const transformContainers = (response, createParent) => {
  if (createParent === undefined) {
    createParent = true;
  }
  const containers = response.containers;
  const type = containers.type;
  const items = containers.container;
  const tree = [];
  let parent;
  if (createParent) {
    parent = {
      title: type,
      badge: [items.length],
      gerbera: {
        id: 0,
        childCount: items.length
      },
      nodes: []
    };

    tree.push(parent);
  }

  for (let i = 0; i < items.length; i++) {
    const item = items[i];
    if (currentView && !currentView.hasClass(item.upnp_class))
      continue;
    const node = {};
    node.title = item.title;
    node.badge = generateBadges(item);
    if (item.child_count > 0) {
      node.nodes = [];
    }
    node.gerbera = {
      id: item.id,
      childCount: item.child_count,
      path: item.location ?? null,
      upnpClass: item.upnp_class ?? null,
      upnpShortcut: item.upnp_shortcut ?? null,
      autoScanMode: item.autoscan_mode,
      autoScanType: item.autoscan_type,
      image: ('image' in item) ? item.image : null
    };
    if (!GerberaApp.serverConfig.enableThumbnail) {
      node.gerbera.image = null;
    }
    if (createParent) {
      parent.nodes.push(node);
    } else {
      tree.push(node);
    }
  }

  return tree;
};

const generateBadges = (item) => {
  const badges = [];
  if (item.child_count > 0) {
    badges.push(item.child_count);
  }

  if (item.autoscan_type === 'ui' || item.autoscan_type === 'persistent') {
    badges.push('a');
  } else if (item.autoscan_type === 'parent') {
    badges.push('p');
  }
  if ('tweak' in item && (item.tweak || item.tweak === 'true')) {
    badges.push('t');
  }
  return badges;
};

const reloadTreeItem = (folderList) => {
  const tree = $('#tree');
  const item = folderList.data('grb-item');
  tree.tree('collapse', folderList);
  return fetchChildren(item.gerbera)
    .then(function (response) {
      var childTree = transformContainers(response, false);
      tree.tree('append', folderList, childTree);
      selectTreeItem(folderList);
      Items.treeItemSelected(item);
    })
    .catch((err) => GerberaApp.error(err));
};

const reloadTreeItemById = (id) => {
  const treeElement = getTreeElementById(id);
  return reloadTreeItem(treeElement);
};

const reloadParentTreeItem = (id) => {
  const treeItem = getTreeElementById(id);
  if (treeItem.length > 0) {
    const parentFolderList = treeItem.parents('ul li').first();
    return reloadTreeItem(parentFolderList);
  }
};

const getTreeElementById = (id) => {
  return $('#tree').tree('getElement', id);
};

export const Tree = {
  destroy,
  getTreeElementById,
  initialize,
  loadTree,
  onTweakEdit,
  onAutoscanEdit,
  reloadParentTreeItem,
  reloadTreeItem,
  reloadTreeItemById,
  selectType,
  viewFactory,
  transformContainers,
  currentTree,
};
