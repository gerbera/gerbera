var GERBERA
if (typeof (GERBERA) === 'undefined') {
  GERBERA = {}
}

GERBERA.Items = (function () {
  'use strict'
  var viewItemsCount
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
    $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      data: {
        req_type: linkType,
        sid: GERBERA.Auth.getSessionId(),
        parent_id: data.gerbera.id,
        start: 0,
        count: viewItems()
      }
    })
      .done(loadItems)
      .fail(GERBERA.App.error)
  }

  var loadItems = function (response) {
    if (response.success) {
      var type = GERBERA.App.getType()
      var items

      if (type === 'db') {
        items = transformItems(response.items.item)
      } else if (type === 'fs') {
        items = transformFiles(response.files.file)
      }

      var datagrid = $('#datagrid')

      if (datagrid.hasClass('grb-dataitems')) {
        datagrid.dataitems('destroy')
      }

      datagrid.dataitems({
        data: items
      })
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

  return {
    initialize: initialize,
    viewItems: viewItems,
    treeItemSelected: treeItemSelected,
    loadItems: loadItems,
    destroy: destroy,
    transformItems: transformItems,
    playVideo: playVideo
  }
})()
