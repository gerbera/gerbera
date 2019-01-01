/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.auth.js - this file is part of Gerbera.

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
/* global md5 */
'use strict'

var GERBERA
if (typeof (GERBERA) === 'undefined') {
  GERBERA = {}
}

GERBERA.Auth = (function () {
  'use strict'
  var LOGGED_IN
  var checkSID = function () {
    return $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      async: false,
      data: {
        req_type: 'auth',
        sid: getSessionId(),
        action: 'get_sid'
      }
    })
      .done(loadSession)
      .fail(GERBERA.App.error)
  }

  var getSessionId = function () {
    return $.cookie('SID')
  }

  var loadSession = function (response) {
    if (!response.sid_was_valid && response.sid && response.sid !== null) {
      $.cookie('SID', response.sid)
      LOGGED_IN = response.logged_in
    } else {
      LOGGED_IN = response.logged_in
    }
  }

  var isLoggedIn = function () {
    return LOGGED_IN
  }

  var authenticate = function () {
    var username = $('#username').val()
    var password = $('#password').val()
    var promise

    if (username.length > 0 && password.length > 0) {
      $('#warning').hide()
      promise =
        $.ajax({
          url: GERBERA.App.clientConfig.api,
          type: 'get',
          data: {
            req_type: 'auth',
            sid: getSessionId(),
            action: 'get_token'
          }
        })
          .done(submitLogin)
          .fail(GERBERA.App.error)
    } else {
      GERBERA.Updates.showMessage('Please enter username and password', undefined, 'warning', 'fa-sign-in')
      promise = $.Deferred().resolve().promise()
    }
    return promise
  }

  var submitLogin = function (response) {
    if (response.success) {
      var username = $('#username').val()
      var password = $('#password').val()
      var token = response.token
      password = md5(token + password)
      return $.ajax({
        url: GERBERA.App.clientConfig.api,
        type: 'get',
        data: {
          req_type: 'auth',
          sid: getSessionId(),
          action: 'login',
          username: username,
          password: password
        }
      })
        .done(checkLogin)
        .fail(GERBERA.App.error)
    } else {
      GERBERA.Updates.showMessage(response.error.text, undefined, 'warning', 'fa-exclamation-circle')
    }
  }

  var checkLogin = function (response) {
    if (response.success) {
      $('.login-field').hide()
      $('#login-submit').hide()
      if (GERBERA.App.serverConfig.accounts) {
        $('#logout').show().click(logout)
      }
      LOGGED_IN = true
      GERBERA.Tree.initialize()
      GERBERA.Items.initialize()
      GERBERA.Trail.initialize()
      GERBERA.Menu.initialize()
      GERBERA.Autoscan.initialize()
      GERBERA.Updates.initialize()
    }
  }

  var logout = function () {
    return $.ajax({
      url: GERBERA.App.clientConfig.api,
      type: 'get',
      async: false,
      data: {
        req_type: 'auth',
        sid: getSessionId(),
        action: 'logout'
      }
    })
      .done(handleLogout)
      .fail(GERBERA.App.error)
  }

  var handleLogout = function () {
    var now = new Date()
    var expire = new Date()
    LOGGED_IN = false
    expire.setTime(now.getTime() - 3600000 * 24 * 360)
    $.cookie('SID', null, {expires: expire})
    GERBERA.App.reload('/index.html')
  }

  return {
    getSessionId: getSessionId,
    checkSID: checkSID,
    isLoggedIn: isLoggedIn,
    authenticate: authenticate,
    logout: logout,
    handleLogout: handleLogout
  }
})()
