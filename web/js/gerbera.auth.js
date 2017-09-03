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
      $('#warning').html('Please enter username and password').show()
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
      GERBERA.Menu.initialize()
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

  var handleLogout = function (response) {
    if (response.success) {
      var now = new Date()
      var expire = new Date()
      LOGGED_IN = false
      expire.setTime(now.getTime() - 3600000 * 24 * 360)
      $.cookie('SID', null, {expires: expire})
      GERBERA.App.reload('/gerbera.html')
    }
  }

  return {
    getSessionId: getSessionId,
    checkSID: checkSID,
    isLoggedIn: isLoggedIn,
    authenticate: authenticate,
    logout: logout
  }
})()
