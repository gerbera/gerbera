/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.trail.js - this file is part of Gerbera.

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

GERBERA.Trail = (function () {
  'use strict'

  var initialize = function () {
    $('#trail').html('')
    return $.Deferred().resolve().promise()
  }

  var destroy = function () {
    var trail = $('#trail')
    if (trail.hasClass('grb-trail')) {
      trail.trail('destroy')
    } else {
      trail.html('')
    }
  }

  var makeTrail = function (selectedItem, config) {
    var items = gatherTrail(selectedItem)
    var configDefaults = {
      itemType: GERBERA.App.getType()
    }
    var trailConfig = $.extend({}, configDefaults, config)
    createTrail(items, trailConfig)
  }

  var makeTrailFromItem = function (items) {
    var treeElement = GERBERA.Tree.getTreeElementById(items.parent_id)
    var itemType = GERBERA.App.getType()
    var enableAdd = false
    var enableEdit = false
    var enableDelete = false
    var enableDeleteAll = false
    var enableAddAutoscan = false
    var enableEditAutoscan = false
    var onAdd
    var onDelete
    var onEdit
    var onAddAutoscan
    var onEditAutoscan
    var onDeleteAll
    var noOp = function () { return false }

    if (itemType === 'db') {
      if (items.parent_id === 0) {
        enableAdd = true
      } else if (items.parent_id === 1) {
        enableEdit = true
        enableAddAutoscan = true
      } else {
        var isVirtual = ('virtual' in items) && items.virtual === true
        var isNotProtected = ('protect_container' in items) && items.protect_container !== true
        var allowsAutoscan = ('autoscan_type' in items) && items.autoscan_type !== 'none'
        enableAdd = isVirtual
        enableEdit = isVirtual
        enableDelete = isNotProtected
        enableDeleteAll = isNotProtected && isVirtual
        enableEditAutoscan = allowsAutoscan && isNotProtected
      }
    } else if (itemType === 'fs') {
      enableAddAutoscan = true
      enableAdd = true
    }

    onAdd = enableAdd ? addItem : noOp
    onEdit = enableEdit ? editItem : noOp
    onDelete = enableDelete ? deleteItem : noOp
    onDeleteAll = enableDeleteAll ? deleteAllItems : noOp
    onAddAutoscan = enableAddAutoscan ? addAutoscan : noOp
    onEditAutoscan = enableEditAutoscan ? addAutoscan : noOp

    var config = {
      enableAdd: enableAdd,
      onAdd: onAdd,
      enableEdit: enableEdit,
      onEdit: onEdit,
      enableDelete: enableDelete,
      onDelete: onDelete,
      enableDeleteAll: enableDeleteAll,
      onDeleteAll: onDeleteAll,
      enableAddAutoscan: enableAddAutoscan,
      onAddAutoscan: onAddAutoscan,
      enableEditAutoscan: enableEditAutoscan,
      onEditAutoscan: onEditAutoscan
    }

    makeTrail(treeElement, config)
  }

  var gatherTrail = function (treeElement) {
    var items = []
    if ($(treeElement).data('grb-id') !== undefined) {
      items.push({
        id: $(treeElement).data('grb-id'),
        text: $(treeElement).children('span.folder-title').text()
      })
    }

    $(treeElement).parents('ul li').each(function (index, element) {
      var title = $(element).children('span.folder-title').text()
      var gerberaId = $(element).data('grb-id')
      var item = {
        id: gerberaId,
        text: title
      }
      items.push(item)
    })
    return items.reverse()
  }

  var createTrail = function (items, config) {
    destroy()
    $('#trail').trail({
      data: items,
      config: config
    })
  }

  var deleteItem = function (event) {
    var item = event.data
    return GERBERA.Items.deleteGerberaItem(item)
      .done(function (response) {
        if (response.success) {
          GERBERA.Updates.showMessage('Successfully removed item', undefined, 'success', 'fa-check')
          GERBERA.Updates.updateTreeByIds(response)
        } else {
          GERBERA.App.error('Failed to remove item')
        }
      })
  }

  var deleteAllItems = function (event) {
    var item = event.data
    return GERBERA.Items.deleteGerberaItem(item, true)
      .done(function (response) {
        if (response.success) {
          GERBERA.Updates.showMessage('Successfully removed all items', undefined, 'success', 'fa-check')
          GERBERA.Updates.updateTreeByIds(response)
        } else {
          GERBERA.App.error('Failed to remove all items')
        }
      })
  }

  var editItem = function (event) {
    return GERBERA.Items.editItem(event)
  }

  var addItem = function (event) {
    var itemType = GERBERA.App.getType()
    if (itemType === 'fs') {
      GERBERA.Items.addFileItem(event)
    } else if (itemType === 'db') {
      GERBERA.Items.addVirtualItem(event)
    }
  }

  var addAutoscan = function (event) {
    GERBERA.Autoscan.addAutoscan(event)
  }

  return {
    initialize: initialize,
    destroy: destroy,
    gatherTrail: gatherTrail,
    deleteItem: deleteItem,
    deleteAllItems: deleteAllItems,
    makeTrail: makeTrail,
    makeTrailFromItem: makeTrailFromItem
  }
})()
