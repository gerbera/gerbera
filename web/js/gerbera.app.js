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

  var error = function (status) {
    console.log('failure: ' + status)
  }

  var initialize = function () {
    return configureDefaults()
      .then(getConfig)
      .then(loadConfig)
      .then(GERBERA.Auth.checkSID)
      .then(displayLogin)
      .then(GERBERA.Menu.initialize)
      .done()
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
    if (response.success) {
      GERBERA.App.serverConfig = response.config
    }
    return $.Deferred().resolve().promise()
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
    } else {
      $('.login-field').show()
      $('#login-submit').click(GERBERA.Auth.authenticate)
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

  return {
    getType: getType,
    setType: setType,
    isTypeDb: isTypeDb,
    clientConfig: clientConfig,
    error: error,
    initialize: initialize,
    reload: reload
  }
})()
