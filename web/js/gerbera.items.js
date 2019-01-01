/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.items.js - this file is part of Gerbera.

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

GERBERA.Items = (function () {
  'use strict'
  var viewItemsCount
  var CURRENT_PAGE = 1
  var initialize = function () {
    var itemsPerPage = GERBERA.App.serverConfig['items-per-page']
    if (itemsPerPage && itemsPerPage.default) {
      viewItemsCount = itemsPerPage.default
    } else {
      viewItemsCount = 25
    }
    $('#datagrid').html('')
    return $.Deferred().resolve().promise()
  }
  var viewItems = function () {
    return viewItemsCount
  }

  var treeItemSelected = function (data) {
    var linkType = (GERBERA.App.getType() === 'db') ? 'items' : 'files'
    GERBERA.Items.currentTreeItem = data
    retrieveGerberaItems(linkType, data.gerbera.id, 0, viewItems())
      .done(loadItems)
      .fail(GERBERA.App.error)
  }

  var retrieveGerberaItems = function (type, parentId, start, count) {
    return $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      data: {
        req_type: type,
        sid: GERBERA.Auth.getSessionId(),
        parent_id: parentId,
        start: start,
        count: count,
        updates: 'check'
      }
    })
  }

  var retrieveItemsForPage = function (event) {
    var pageItem = event.data
    var linkType = (GERBERA.App.getType() === 'db') ? 'items' : 'files'
    var start = (pageItem.pageNumber * pageItem.itemsPerPage) - pageItem.itemsPerPage
    return retrieveGerberaItems(linkType, pageItem.parentId, start, viewItems())
      .then(loadItems)
      .done(function () {
        setPage(pageItem.pageNumber)
      })
      .fail(GERBERA.App.error)
  }

  var nextPage = function (event) {
    var pageItem = event.data
    var pageNumber = currentPage() + 1
    var start = (pageNumber * pageItem.itemsPerPage) - pageItem.itemsPerPage
    if (start < pageItem.totalMatches) {
      var pageEvent = {
        data: {
          pageNumber: pageNumber,
          itemsPerPage: pageItem.itemsPerPage,
          parentId: pageItem.parentId
        }
      }
      return retrieveItemsForPage(pageEvent)
    } else {
      return $.Deferred().resolve().promise()
    }
  }

  var previousPage = function (event) {
    var pageItem = event.data
    var pageNumber = currentPage() - 1
    var start = (pageNumber * pageItem.itemsPerPage) - pageItem.itemsPerPage
    if (start >= 0) {
      var pageEvent = {
        data: {
          pageNumber: pageNumber,
          itemsPerPage: pageItem.itemsPerPage,
          parentId: pageItem.parentId
        }
      }
      return retrieveItemsForPage(pageEvent)
    } else {
      return $.Deferred().resolve().promise()
    }
  }

  var editItem = function (event) {
    var item = event.data
    if (item) {
      $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: {
          req_type: 'edit_load',
          sid: GERBERA.Auth.getSessionId(),
          object_id: item.id,
          updates: 'check'
        }
      })
        .done(loadEditItem)
        .fail(GERBERA.App.error)
    }
  }

  var loadEditItem = function (response) {
    var editModal = $('#editModal')
    if (response.success) {
      editModal.editmodal('loadItem', {
        item: response.item,
        onSave: saveItem
      })

      editModal.editmodal('show')
      GERBERA.Updates.updateTreeByIds(response)
    }
  }

  var saveItem = function () {
    var item = $('#editModal').editmodal('saveItem')
    var saveData = {
      req_type: 'edit_save',
      sid: GERBERA.Auth.getSessionId(),
      updates: 'check'
    }
    var requestData = $.extend({}, item, saveData)

    if (requestData.object_id && requestData.object_id > 0) {
      $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: requestData
      })
        .done(saveItemComplete)
        .fail(GERBERA.App.error)
    }
  }

  var saveItemComplete = function (response) {
    if (response.success && GERBERA.Items.currentTreeItem) {
      // TODO: better served as an event to refresh
      var editModal = $('#editModal')
      editModal.editmodal('hide')
      editModal.editmodal('reset')
      GERBERA.Updates.showMessage('Successfully saved item', undefined, 'success', 'fa-check')
      GERBERA.Updates.updateTreeByIds(response)
    } else {
      GERBERA.App.error('Failed to save item')
    }
  }

  var deleteItemFromList = function (event) {
    var item = event.data
    if (item) {
      deleteGerberaItem(item)
        .done(deleteComplete)
        .fail(GERBERA.App.error)
    }
  }

  var deleteGerberaItem = function (item, includeAll) {
    var deleteAll = includeAll ? 1 : 0
    return $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      data: {
        req_type: 'remove',
        sid: GERBERA.Auth.getSessionId(),
        object_id: item.id,
        all: deleteAll,
        updates: 'check'
      }
    })
  }

  var deleteComplete = function (response) {
    if (response.success && GERBERA.Items.currentTreeItem) {
      // TODO: better served as an event to refresh
      GERBERA.Updates.showMessage('Successfully removed item', undefined, 'success', 'fa-check')
      GERBERA.Items.treeItemSelected(GERBERA.Items.currentTreeItem)
      GERBERA.Updates.updateTreeByIds(response)
    } else {
      GERBERA.App.error('Failed to delete item')
    }
  }

  var downloadItem = function (event) {
    var item = event.data
    event.preventDefault()
    // TODO: content type
    window.location.href = item.url
  }

  var addFileItem = function (event) {
    var item = event.data
    if (item) {
      $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: {
          req_type: 'add',
          sid: GERBERA.Auth.getSessionId(),
          object_id: item.id
        }
      })
        .done(addFileItemComplete)
        .fail(GERBERA.App.error)
    }
  }

  var addFileItemComplete = function (response) {
    if (response.success) {
      GERBERA.Updates.showMessage('Successfully added item', undefined, 'success', 'fa-check')
      GERBERA.Updates.addUiTimer()
    } else {
      GERBERA.App.error('Failed to add item')
    }
  }

  var addVirtualItem = function (event) {
    var item = event.data
    var editModal = $('#editModal')
    if (item) {
      editModal.editmodal('addNewItem', {type: 'container', item: item, onSave: addObject})
      editModal.editmodal('show')
    }
  }

  var addObject = function () {
    var item = $('#editModal').editmodal('addObject')
    var addObjectData = {
      req_type: 'add_object',
      sid: GERBERA.Auth.getSessionId(),
      updates: 'check'
    }
    var requestData = $.extend({}, item, addObjectData)

    if (requestData.parent_id && requestData.parent_id >= 0) {
      $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: requestData
      })
        .done(addObjectComplete)
        .fail(GERBERA.App.error)
    }
  }

  var addObjectComplete = function (response) {
    if (response.success && GERBERA.Items.currentTreeItem) {
      // TODO: better served as an event to refresh
      var editModal = $('#editModal')
      editModal.editmodal('hide')
      editModal.editmodal('reset')
      GERBERA.Updates.showMessage('Successfully added object', undefined, 'success', 'fa-check')
      GERBERA.Items.treeItemSelected(GERBERA.Items.currentTreeItem)
      GERBERA.Updates.updateTreeByIds(response)
    } else {
      GERBERA.App.error('Failed to add object')
    }
  }

  var loadItems = function (response) {
    if (response.success) {
      var type = GERBERA.App.getType()
      var items
      var parentItem
      var pager

      if (type === 'db') {
        items = transformItems(response.items.item)
        parentItem = response.items
        setPage(1) // reset page
        pager = {
          currentPage: Math.ceil(response.items.start / GERBERA.Items.viewItems()) + 1,
          pageCount: 10,
          onClick: GERBERA.Items.retrieveItemsForPage,
          onNext: GERBERA.Items.nextPage,
          onPrevious: GERBERA.Items.previousPage,
          totalMatches: response.items.total_matches,
          itemsPerPage: GERBERA.Items.viewItems(),
          parentId: response.items.parent_id
        }
      } else if (type === 'fs') {
        items = transformFiles(response.files.file)
        parentItem = response.files
      }

      var datagrid = $('#datagrid')

      if (datagrid.hasClass('grb-dataitems')) {
        datagrid.dataitems('destroy')
      }

      datagrid.dataitems({
        data: items,
        pager: pager,
        onEdit: editItem,
        onDelete: deleteItemFromList,
        onDownload: downloadItem,
        onAdd: addFileItem,
        itemType: GERBERA.App.getType()
      })

      GERBERA.Trail.makeTrailFromItem(parentItem)
    }
  }

  var transformItems = function (items) {
    var widgetItems = []
    for (var i = 0; i < items.length; i++) {
      var gItem = items[i]
      var item = {
        id: gItem.id,
        text: gItem.title,
        url: gItem.res
      }

      if (GERBERA.App.serverConfig.enableThumbnail) {
        item.img = '/content/media/object_id/' + gItem.id + '/res_id/1/rh/6/ext/file.jpg'
      }
      if (GERBERA.App.serverConfig.enableVideo) {
        item.video = '/content/media/object_id/' + gItem.id + '/res_id/0/ext/file.mp4'
      }

      widgetItems.push(item)
    }
    return widgetItems
  }

  var transformFiles = function (files) {
    var widgetItems = []
    for (var i = 0; i < files.length; i++) {
      var gItem = files[i]
      var item = {
        id: gItem.id,
        text: gItem.filename
      }
      widgetItems.push(item)
    }
    return widgetItems
  }

  var destroy = function () {
    var datagrid = $('#datagrid')
    if (datagrid.hasClass('grb-dataitems')) {
      datagrid.dataitems('destroy')
    } else {
      datagrid.html('')
    }
  }

  var playVideo = function (event) {
    var item = event.data.item
    var container = $('#video')

    container.html('')
    var video = $('<video></video>')
    video.attr('height', '480')
    video.attr('width', '720')

    var source = $('<source></source>')
    source.attr('src', item.video)
    source.attr('type', 'video/mp4')

    source.appendTo(video)
    video.appendTo(container)
    container.css('display', 'block')
  }

  var currentPage = function () {
    return CURRENT_PAGE
  }

  var setPage = function (pageNumber) {
    CURRENT_PAGE = pageNumber
  }

  return {
    initialize: initialize,
    viewItems: viewItems,
    treeItemSelected: treeItemSelected,
    loadItems: loadItems,
    destroy: destroy,
    transformItems: transformItems,
    retrieveGerberaItems: retrieveGerberaItems,
    retrieveItemsForPage: retrieveItemsForPage,
    nextPage: nextPage,
    previousPage: previousPage,
    playVideo: playVideo,
    editItem: editItem,
    saveItem: saveItem,
    saveItemComplete: saveItemComplete,
    loadEditItem: loadEditItem,
    deleteItemFromList: deleteItemFromList,
    deleteGerberaItem: deleteGerberaItem,
    deleteComplete: deleteComplete,
    downloadItem: downloadItem,
    addFileItem: addFileItem,
    addFileItemComplete: addFileItemComplete,
    addVirtualItem: addVirtualItem,
    addObject: addObject,
    currentPage: currentPage,
    setPage: setPage
  }
})()
