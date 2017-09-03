/* global GERBERA jasmine it expect spyOn describe beforeEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera Auth', function () {
  describe('checkSID()', function () {
    var ajaxSpy, mockGetSid
    beforeEach(function () {
      loadJSONFixtures('get_sid.not.logged_in.json')
      mockGetSid = getJSONFixture('get_sid.not.logged_in.json')

      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        d.resolve(mockGetSid)
        return d.promise()
      })
    })

    it('calls the server for a new SID and sets the cookie for SID', function (done) {
      GERBERA.Auth.checkSID().then(function () {
        expect($.cookie('SID')).toBe('563806f88aea6b33429ebdb85ce14beb')
        expect(GERBERA.Auth.isLoggedIn()).toBeFalsy()
        done()
      })
    })

    it('calls GERBERA.App.error when the server call fails', function (done) {
      spyOn(GERBERA.App, 'error')
      ajaxSpy.and.callFake(function (options) {
        var d = $.Deferred()
        d.reject({})
        return d.promise()
      })
      GERBERA.Auth.checkSID().fail(function () {
        expect(GERBERA.App.error).toHaveBeenCalled()
        done()
      })
    })
  })

  describe('SID()', function () {
    it('retrieves the session ID from the cookie', function () {
      spyOn($, 'cookie').and.returnValue('A_MOCK_SESSION_ID')
      expect(GERBERA.Auth.getSessionId()).toEqual('A_MOCK_SESSION_ID')
    })
  })

  describe('authenticate()', function () {
    var mockToken, mockLogin, ajaxSpy
    beforeEach(function () {
      loadJSONFixtures('get_token.success.json')
      mockToken = getJSONFixture('get_token.success.json')
      mockLogin = getJSONFixture('login.success.json')
      loadFixtures('index.html')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        if (options.data.action === 'get_token') {
          d.resolve(mockToken)
        } else if (options.data.action === 'login') {
          d.resolve(mockLogin)
        }
        return d.promise()
      })
    })

    it('calls the server to get a token and then logs the user into the system', function (done) {
      $('#username').val('user_name')
      $('#password').val('password')
      GERBERA.App.serverConfig = {
        accounts: true
      }

      GERBERA.Auth.authenticate().then(function () {
        expect(GERBERA.Auth.isLoggedIn()).toBeTruthy()
        expect(ajaxSpy.calls.count()).toBe(2)
        done()
      })
    })
  })

  describe('logout()', function () {
    beforeEach(function () {
      spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        d.resolve({success: true})
        return d.promise()
      })
      loadFixtures('index.html')
      $.cookie('SID', '12345')
    })

    it('calls the server to logout the session and expires the session ID cookie', function (done) {
      spyOn(GERBERA.App, 'reload')
      GERBERA.Auth.logout().then(function () {
        expect(GERBERA.Auth.isLoggedIn()).toBeFalsy()
        expect(GERBERA.Auth.getSessionId()).toBeUndefined()
        expect($('#nav-db').attr('class')).toBe('navbar-link disabled')
        expect($('#nav-fs').attr('class')).toBe('navbar-link disabled')
        expect(GERBERA.App.reload).toHaveBeenCalledWith('/gerbera.html')
        done()
      })
    })
  })
})
