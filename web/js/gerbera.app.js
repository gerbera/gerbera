/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.app.js - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2018 Gerbera Contributors

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

GERBERA.App = (function () {
  'use strict'
  var clientConfig = {
    api: 'content/interface'
  }

  var isTypeDb = function () {
    return getType() === 'db'
  }

  var getType = function () {
    return $.cookie('TYPE')
  }

  var setType = function (type) {
    $.cookie('TYPE', type)
  }

  var error = function (event) {
    var message
    if (typeof event === 'string') {
      message = event
    } else if (event && event.responseText) {
      message = event.responseText
    } else if (event && event.error) {
      message = event.error.text
    } else {
      message = 'The system encountered an error'
    }
    GERBERA.Updates.showMessage(message)
  }

  var initialize = function () {
    GERBERA.Updates.initialize()

    return configureDefaults()
      .then(getConfig)
      .then(loadConfig)
      .then(GERBERA.Auth.checkSID)
      .then(displayLogin)
      .then(GERBERA.Menu.initialize)
      .done()
      .fail(GERBERA.App.error)
  }

  var configureDefaults = function () {
    if (getType() === undefined) {
      setType('db')
    }
    return $.Deferred().resolve().promise()
  }

  var getConfig = function () {
    return $.ajax({
      url: clientConfig.api,
      type: 'get',
      async: false,
      data: {
        req_type: 'auth',
        sid: GERBERA.Auth.getSessionId(),
        action: 'get_config'
      }
    })
  }

  var loadConfig = function (response) {
    var deferred;
    if (response.success) {
      GERBERA.App.serverConfig = response.config
      var pollingInterval = response.config['poll-interval']
      GERBERA.App.serverConfig['poll-interval'] = parseInt(pollingInterval) * 1000
      deferred = $.Deferred().resolve().promise()
    } else {
      deferred = $.Deferred().reject(response).promise()
    }
    return deferred
  }

  var displayLogin = function () {
    if (GERBERA.Auth.isLoggedIn()) {
      $('.login-field').hide()
      $('#login-submit').hide()
      if (GERBERA.App.serverConfig.accounts) {
        $('#logout').show().click(GERBERA.Auth.logout)
      }
      GERBERA.Items.initialize()
      GERBERA.Tree.initialize()
      GERBERA.Trail.initialize()
      GERBERA.Autoscan.initialize()
      GERBERA.Updates.initialize()
    } else {
      $('.login-field').show()
      $('#login-form').submit(function (event) {
        GERBERA.Auth.authenticate(event)
        event.preventDefault()
      })
      $('#logout').hide()
    }
    return $.Deferred().resolve().promise()
  }

  var reload = function (path) {
    window.location = path
  }

  var disable = function () {
    GERBERA.Menu.disable()
    GERBERA.Menu.hideLogin()
    GERBERA.Menu.hideMenu()
    GERBERA.Tree.destroy()
    GERBERA.Trail.destroy()
    GERBERA.Items.destroy()
  }

  return {
    getType: getType,
    setType: setType,
    isTypeDb: isTypeDb,
    clientConfig: clientConfig,
    error: error,
    initialize: initialize,
    reload: reload,
    disable: disable
  }
})()
