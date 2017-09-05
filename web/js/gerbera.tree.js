var GERBERA
if (typeof (GERBERA) === 'undefined') {
  GERBERA = {}
}

GERBERA.Tree = (function () {
  'use strict'

  var currentTree = []
  var currentType = 'db'

  var onItemSelected = function (event) {
    generateBreadCrumb(event.target)
    selectTreeItem(event.target)
    GERBERA.Items.treeItemSelected(event.data)
  }

  var onItemExpanded = function (event) {
    if (event.data.gerbera && event.data.gerbera.id) {
      var tree = $('#tree')
      if (tree.tree('closed', event.target) && event.data.gerbera.childCount > 0) {
        fetchChildren(event.data.gerbera).then(function (response) {
          var childTree = transformContainers(response, false)
          tree.tree('append', $(event.target.parentElement), childTree)
        })
      } else {
        tree.tree('collapse', $(event.target.parentElement))
      }
    }
  }

  var selectTreeItem = function (target) {
    var tree = $('#tree')
    tree.tree('select', target)
  }

  var generateBreadCrumb = function (node) {
    var itemBreadcrumb = $('#item-breadcrumb')
    itemBreadcrumb.html('')
    $(node).parents('ul li').each(function (index, element) {
      var title = $(element).children('span.folder-title')
      var crumb = $('<li><a></a></li>')
      crumb.children('a').text(title.text())
      itemBreadcrumb.prepend(crumb)
    })
    itemBreadcrumb.show()
  }

  var treeViewConfig = {
    titleClass: 'folder-title',
    closedIcon: 'glyphicon glyphicon-folder-close',
    openIcon: 'glyphicon glyphicon-folder-open',
    onSelection: onItemSelected,
    onExpand: onItemExpanded
  }

  var initialize = function () {
    $('#tree').html('')
    $('#item-breadcrumb').html('<li>Select a type to begin</li>')
    return $.Deferred().resolve().promise()
  }

  var selectType = function (type, parentId) {
    currentType = type
    var linkType = (currentType === 'db') ? 'containers' : 'directories'
    $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      data: {
        req_type: linkType,
        sid: GERBERA.Auth.getSessionId(),
        parent_id: parentId,
        select_it: 0
      }
    })
      .done(loadTree)
      .fail(GERBERA.App.error)
  }

  var loadTree = function (response, config) {
    var tree = $('#tree')
    config = $.extend({}, treeViewConfig, config)
    if (response.success) {
      if (tree.hasClass('grb-tree')) {
        tree.tree('destroy')
      }
      $('#item-breadcrumb').html('')

      currentTree = transformContainers(response)
      tree.tree({
        data: currentTree,
        config: config
      })

      generateBreadCrumb($('#tree span.folder-title').first())
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
      node.badge = item.child_count > 0 ? [item.child_count] : []
      if (item.child_count > 0) {
        node.nodes = []
      }
      node.gerbera = {
        id: item.id,
        childCount: item.child_count
      }
      if (createParent) {
        parent.nodes.push(node)
      } else {
        tree.push(node)
      }
    }

    return tree
  }

  var destroy = function () {
    currentTree = []
    currentType = 'db'
    if($('#tree').hasClass('grb-tree')) {
      $('#tree').tree('destroy')
    }
    return $.Deferred().resolve().promise()
  }

  return {
    initialize: initialize,
    selectType: selectType,
    destroy: destroy,
    transformContainers: transformContainers,
    treeViewConfig: treeViewConfig,
    loadTree: loadTree
  }
})()
