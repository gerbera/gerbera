/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.updates.js - this file is part of Gerbera.

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

GERBERA.Updates = (function () {
  'use strict'

  var POLLING_INTERVAL
  var UI_TIMEOUT

  var initialize = function () {
    $('#toast').toast()

    $(document).ajaxComplete(errorCheck)

    return $.Deferred().resolve().promise()
  }

  var errorCheck = function (event, xhr) {
    var response = xhr.responseJSON
    if (response && !response.success) {
      if (response.error) {
        showMessage(response.error.text, undefined, 'danger', 'fa-exclamation-triangle')
        if(response.error.code === '900') {
          GERBERA.App.disable()
        } else if(response.error.code === '400') {
          GERBERA.Auth.handleLogout()
        }
      }
    }
  }

  var showMessage = function (message, callback, type, icon) {
    var toast = {message: message, type: type, icon: icon}
    if (callback) {
      toast.callback = callback
    }
    $('#toast').toast('show', toast)
  }

  var showTask = function (message, callback, type, icon) {
    var toast = {message: message, type: type, icon: icon}
    if (callback) {
      toast.callback = callback
    }
    $('#toast').toast('showTask', toast)
  }

  var getUpdates = function (force) {
    if (GERBERA.Auth.isLoggedIn()) {
      var requestData = {
        req_type: 'void',
        sid: GERBERA.Auth.getSessionId()
      }

      var checkUpdates
      if (GERBERA.App.getType() !== 'db') {
        checkUpdates = {}
      } else {
        checkUpdates = {
          updates: (force ? 'get' : 'check')
        }
      }
      requestData = $.extend({}, requestData, checkUpdates)

      return $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: requestData
      })
        .then(GERBERA.Updates.updateTask)
        .done(GERBERA.Updates.updateUi)
        .fail(GERBERA.Updates.clearAll)
    } else {
      return $.Deferred().resolve().promise()
    }
  }

  var updateTask = function (response) {
    var promise
    if (response.success) {
      if (response.task) {
        var taskId = response.task.id
        if (taskId === -1) {
          promise = GERBERA.Updates.clearTaskInterval(response)
        } else {
          showTask(response.task.text, undefined, 'info', 'fa-refresh fa-spin fa-fw')
          GERBERA.Updates.addTaskInterval()
          promise = $.Deferred().resolve(response).promise()
        }
      } else {
        promise = GERBERA.Updates.clearTaskInterval(response)
      }
    } else {
      promise = $.Deferred().resolve(response).promise()
    }
    return promise
  }

  var updateUi = function (response) {
    if (response.success) {
      updateTreeByIds(response)
    }

    return $.Deferred().resolve(response).promise()
  }

  var clearTaskInterval = function (response) {
    if (GERBERA.Updates.isPolling()) {
      window.clearInterval(POLLING_INTERVAL)
      POLLING_INTERVAL = false
    }
    return $.Deferred().resolve(response).promise()
  }

  var clearUiTimer = function (response) {
    if (GERBERA.Updates.isTimer()) {
      window.clearTimeout(UI_TIMEOUT)
      UI_TIMEOUT = false
    }
    return $.Deferred().resolve(response).promise()
  }

  var clearAll = function (response) {
    GERBERA.Updates.clearUiTimer(response)
    GERBERA.Updates.clearTaskInterval(response)
  }

  var addUiTimer = function (interval) {
    var timeoutInterval = interval || GERBERA.App.serverConfig['poll-interval']
    if (!GERBERA.Updates.isTimer()) {
      UI_TIMEOUT = window.setTimeout(function () {
        GERBERA.Updates.getUpdates(true)
      }, timeoutInterval)
    }
  }

  var addTaskInterval = function () {
    if (!GERBERA.Updates.isPolling()) {
      POLLING_INTERVAL = window.setInterval(function () {
        GERBERA.Updates.getUpdates(false)
      }, GERBERA.App.serverConfig['poll-interval'])
    }
  }

  var isTimer = function () {
    return UI_TIMEOUT
  }

  var isPolling = function () {
    return POLLING_INTERVAL
  }

  var updateTreeByIds = function (response) {
    if (response && response.success) {
      if (response.update_ids) {
        var updateIds = response.update_ids
        if (updateIds.pending) {
          GERBERA.Updates.addUiTimer(800)
        } else if (updateIds.updates === false) {
          GERBERA.Updates.clearUiTimer(response)
        } else {
          if (updateIds.ids === 'all') {
            GERBERA.Tree.reloadTreeItemById('0')
            GERBERA.Updates.clearUiTimer()
          } else if (updateIds.ids && updateIds.ids.length > 0) {
            var idList = updateIds.ids.split(',')
            if ($.inArray('0', idList) > -1) {
              GERBERA.Tree.reloadTreeItemById('0')
            } else {
              for (var i = 0; i < idList.length; i++) {
                var idToReload = idList[i]
                GERBERA.Tree.reloadParentTreeItem(idToReload)
              }
            }
            GERBERA.Updates.clearUiTimer()
          } else {
            GERBERA.Updates.addUiTimer(800)
          }
        }
      } else {
        GERBERA.Updates.clearUiTimer()
      }
    }
  }

  return {
    initialize: initialize,
    showMessage: showMessage,
    getUpdates: getUpdates,
    updateTask: updateTask,
    updateUi: updateUi,
    clearTaskInterval: clearTaskInterval,
    clearUiTimer: clearUiTimer,
    clearAll: clearAll,
    addUiTimer: addUiTimer,
    addTaskInterval: addTaskInterval,
    isPolling: isPolling,
    isTimer: isTimer,
    updateTreeByIds: updateTreeByIds,
    errorCheck: errorCheck
  }
})()
