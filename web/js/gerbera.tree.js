/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.tree.js - this file is part of Gerbera.

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
var GERBERA
if (typeof (GERBERA) === 'undefined') {
  GERBERA = {}
}

GERBERA.Tree = (function () {
  'use strict'

  var currentTree = []
  var currentType = 'db'

  var treeViewCss = {
    titleClass: 'folder-title',
    closedIcon: 'fa fa-folder',
    openIcon: 'fa fa-folder-open'
  }

  var initialize = function () {
    $('#tree').html('')
    return $.Deferred().resolve().promise()
  }

  var destroy = function () {
    currentTree = []
    currentType = 'db'
    if ($('#tree').hasClass('grb-tree')) {
      $('#tree').tree('destroy')
    }
    return $.Deferred().resolve().promise()
  }

  var selectType = function (type, parentId) {
    currentType = type
    var linkType = (currentType === 'db') ? 'containers' : 'directories'

    return $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      data: {
        req_type: linkType,
        sid: GERBERA.Auth.getSessionId(),
        parent_id: parentId,
        select_it: 0
      }
    })
      .then(loadTree)
      .done(checkForUpdates)
      .fail(GERBERA.App.error)
  }

  var onItemSelected = function (event) {
    var folderList = event.target.parentElement
    var item = event.data

    selectTreeItem(folderList)
    GERBERA.Items.treeItemSelected(item)
  }

  var onItemExpanded = function (event) {
    var tree = $('#tree')
    var folderName = event.target
    var folderList = event.target.parentElement
    var item = event.data

    if (item.gerbera && item.gerbera.id) {
      if (tree.tree('closed', folderName) && item.gerbera.childCount > 0) {
        fetchChildren(item.gerbera).then(function (response) {
          var childTree = transformContainers(response, false)
          tree.tree('append', $(folderList), childTree)
        })
      } else {
        tree.tree('collapse', $(folderList))
      }
    }
  }

  var onAutoscanEdit = function (event) {
    GERBERA.Autoscan.addAutoscan(event)
  }

  var selectTreeItem = function (folderList) {
    var tree = $('#tree')
    tree.tree('select', folderList)
  }

  var loadTree = function (response, config) {
    var tree = $('#tree')
    var treeViewEvents = {
      onSelection: onItemSelected,
      onExpand: onItemExpanded,
      onAutoscanEdit: onAutoscanEdit
    }

    config = $.extend({}, treeViewCss, treeViewEvents, config)
    if (response.success) {
      if (tree.hasClass('grb-tree')) {
        tree.tree('destroy')
      }

      GERBERA.Trail.destroy()

      currentTree = transformContainers(response)
      tree.tree({
        data: currentTree,
        config: config
      })

      GERBERA.Trail.makeTrail($('#tree span.folder-title').first())
    }
  }

  var fetchChildren = function (gerberaData) {
    currentType = GERBERA.App.getType()
    var linkType = (currentType === 'db') ? 'containers' : 'directories'
    return $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      data: {
        req_type: linkType,
        sid: GERBERA.Auth.getSessionId(),
        parent_id: gerberaData.id,
        select_it: 0
      }
    })
      .fail(GERBERA.App.error)
  }

  var transformContainers = function (response, createParent) {
    if (createParent === undefined) {
      createParent = true
    }
    var containers = response.containers
    var type = containers.type
    var items = containers.container
    var tree = []

    if (createParent) {
      var parent = {
        title: type,
        badge: [items.length],
        gerbera: {
          id: 0,
          childCount: items.length
        },
        nodes: []
      }

      tree.push(parent)
    }

    for (var i = 0; i < items.length; i++) {
      var item = items[i]
      var node = {}
      node.title = item.title
      node.badge = generateBadges(item)
      if (item.child_count > 0) {
        node.nodes = []
      }
      node.gerbera = {
        id: item.id,
        childCount: item.child_count,
        autoScanMode: item.autoscan_mode,
        autoScanType: item.autoscan_type
      }
      if (createParent) {
        parent.nodes.push(node)
      } else {
        tree.push(node)
      }
    }

    return tree
  }

  var generateBadges = function (item) {
    var badges = []
    if (item.child_count > 0) {
      badges.push(item.child_count)
    }

    if (item.autoscan_type === 'ui' || item.autoscan_type === 'persistent') {
      badges.push('a')
    }

    return badges
  }

  var checkForUpdates = function () {
    if (GERBERA.App.getType() === 'db') {
      return GERBERA.Updates.getUpdates(true)
    } else {
      return $.Deferred().resolve().promise()
    }
  }

  var reloadTreeItem = function (folderList) {
    var tree = $('#tree')
    var item = folderList.data('grb-item')
    tree.tree('collapse', folderList)
    return fetchChildren(item.gerbera).then(function (response) {
      var childTree = transformContainers(response, false)
      tree.tree('append', folderList, childTree)
      selectTreeItem(folderList)
      GERBERA.Items.treeItemSelected(item)
    })
  }

  var reloadTreeItemById = function (id) {
    var treeElement = getTreeElementById(id)
    return reloadTreeItem(treeElement)
  }

  var reloadParentTreeItem = function (id) {
    var treeItem = getTreeElementById(id)
    if (treeItem.length > 0) {
      var parentFolderList = treeItem.parents('ul li').first()
      return reloadTreeItem(parentFolderList)
    }
  }

  var getTreeElementById = function (id) {
    return $('#tree').tree('getElement', id)
  }

  return {
    initialize: initialize,
    selectType: selectType,
    destroy: destroy,
    getTreeElementById: getTreeElementById,
    reloadTreeItem: reloadTreeItem,
    reloadTreeItemById: reloadTreeItemById,
    reloadParentTreeItem: reloadParentTreeItem,
    onAutoscanEdit: onAutoscanEdit,
    transformContainers: transformContainers,
    loadTree: loadTree
  }
})()
