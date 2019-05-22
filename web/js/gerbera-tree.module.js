/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-tree.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2019 Gerbera Contributors

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
import {Autoscan} from "./gerbera-autoscan.module.js";
import {Auth} from "./gerbera-auth.module.js";
import {GerberaApp} from "./gerbera-app.module.js";
import {Trail} from "./gerbera-trail.module.js";
import {Items} from "./gerbera-items.module.js";
import {Updates} from "./gerbera-updates.module.js";

const treeViewCss = {
  titleClass: 'folder-title',
  closedIcon: 'fa fa-folder',
  openIcon: 'fa fa-folder-open'
};

let currentTree = [];
let currentType = 'db';

const destroy = () => {
  currentTree = [];
  currentType = 'db';
  const tree = $('#tree');
  if (tree.hasClass('grb-tree')) {
    tree.tree('destroy');
  }
  return Promise.resolve();
};

const initialize = () => {
  $('#tree').html('');
  return Promise.resolve();
};

const selectType = (type, parentId) => {
  currentType = type;
  const linkType = (currentType === 'db') ? 'containers' : 'directories';

  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: {
      req_type: linkType,
      sid: Auth.getSessionId(),
      parent_id: parentId,
      select_it: 0
    }
  })
    .then((response) => loadTree(response))
    .then(() => checkForUpdates())
    .catch((err) => GerberaApp.error(err))
};

const onItemSelected = (event) => {
  const folderList = event.target.parentElement;
  const item = event.data;

  selectTreeItem(folderList);
  Items.treeItemSelected(item);
};

const onItemExpanded = (event) => {
  const tree = $('#tree');
  const folderName = event.target;
  const folderList = event.target.parentElement;
  const item = event.data;

  if (item.gerbera && item.gerbera.id) {
    if (tree.tree('closed', folderName) && item.gerbera.childCount > 0) {
      fetchChildren(item.gerbera).then(function (response) {
        var childTree = transformContainers(response, false);
        tree.tree('append', $(folderList), childTree);
      })
    } else {
      tree.tree('collapse', $(folderList));
    }
  }
};

const onAutoscanEdit = (event) => {
  Autoscan.addAutoscan(event);
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
    onAutoscanEdit: onAutoscanEdit
  };

  config = $.extend({}, treeViewCss, treeViewEvents, config);
  if (response.success) {
    if (tree.hasClass('grb-tree')) {
      tree.tree('destroy')
    }

    Trail.destroy();

    currentTree = transformContainers(response);
    tree.tree({
      data: currentTree,
      config: config
    });

    Trail.makeTrail($('#tree span.folder-title').first());
  }
};

const fetchChildren = (gerberaData) => {
  currentType = GerberaApp.getType();
  const linkType = (currentType === 'db') ? 'containers' : 'directories';
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: {
      req_type: linkType,
      sid: Auth.getSessionId(),
      parent_id: gerberaData.id,
      select_it: 0
    }
  })
    .catch((err) => GerberaApp.error(err));
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
    const node = {};
    node.title = item.title;
    node.badge = generateBadges(item);
    if (item.child_count > 0) {
      node.nodes = [];
    }
    node.gerbera = {
      id: item.id,
      childCount: item.child_count,
      autoScanMode: item.autoscan_mode,
      autoScanType: item.autoscan_type
    };
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
  }

  return badges;
};

const reloadTreeItem = (folderList) => {
  const tree = $('#tree');
  const item = folderList.data('grb-item');
  tree.tree('collapse', folderList);
  return fetchChildren(item.gerbera).then(function (response) {
    var childTree = transformContainers(response, false);
    tree.tree('append', folderList, childTree);
    selectTreeItem(folderList);
    Items.treeItemSelected(item);
  });
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

const checkForUpdates = () => {
  if (GerberaApp.getType() === 'db') {
    return Updates.getUpdates(true);
  } else {
    return Promise.resolve();
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
  onAutoscanEdit,
  reloadParentTreeItem,
  reloadTreeItem,
  reloadTreeItemById,
  selectType,
  transformContainers,
  currentTree,
};