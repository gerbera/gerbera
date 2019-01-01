/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.autoscan.js - this file is part of Gerbera.

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

GERBERA.Autoscan = (function () {
  var initialize = function () {
    $('#autoscanModal').autoscanmodal('reset')
  }

  var addAutoscan = function (event) {
    var item = event.data
    var fromFs = GERBERA.App.getType() === 'fs'
    if (item) {
      var requestData = {
        req_type: 'autoscan',
        sid: GERBERA.Auth.getSessionId(),
        object_id: item.id,
        action: 'as_edit_load',
        from_fs: fromFs
      }

      if (GERBERA.App.getType() === 'db') {
        requestData = $.extend({}, requestData, {updates: 'check'})
      }

      $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: requestData
      })
        .done(loadNewAutoscan)
        .fail(GERBERA.App.error)
    }
  }

  var loadNewAutoscan = function (response) {
    var autoscanModal = $('#autoscanModal')
    if (response.success) {
      autoscanModal.autoscanmodal('loadItem', {item: response.autoscan, onSave: submitAutoscan})
      autoscanModal.autoscanmodal('show')
      GERBERA.Updates.updateTreeByIds(response)
    }
  }

  var submitAutoscan = function () {
    var autoscanModal = $('#autoscanModal')
    var item = autoscanModal.autoscanmodal('saveItem')
    var request = {
      sid: GERBERA.Auth.getSessionId(),
      req_type: 'autoscan',
      action: 'as_edit_save'
    }
    var requestData = $.extend({}, request, item)

    if (item) {
      $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: requestData
      })
        .done(submitComplete)
        .fail(GERBERA.App.error)
    }
  }

  var submitComplete = function (response) {
    var msg
    if (response.success) {
      if (response.task) {
        msg = response.task.text
      } else {
        msg = 'Successfully submitted autoscan'
      }
      GERBERA.Updates.showMessage(msg, undefined, 'success', 'fa-check')
      GERBERA.Updates.getUpdates(false)
    } else {
      GERBERA.App.error('Failed to submit autoscan')
    }
  }

  return {
    initialize: initialize,
    addAutoscan: addAutoscan,
    loadNewAutoscan: loadNewAutoscan,
    submitAutoscan: submitAutoscan,
    submitComplete: submitComplete
  }
})()
