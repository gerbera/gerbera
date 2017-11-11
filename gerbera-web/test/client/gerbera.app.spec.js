/* global GERBERA jasmine it expect spyOn describe beforeEach beforeAll afterEach loadJSONFixtures getJSONFixture */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera UI App', function () {
  describe('initialize()', function () {
    var mockConfig, convertedConfig, ajaxSpy, cookieSpy

    beforeAll(function () {
      loadJSONFixtures('converted-config.json')
      loadJSONFixtures('config.json')
      mockConfig = getJSONFixture('config.json')
      convertedConfig = getJSONFixture('converted-config.json')
      ajaxSpy = spyOn($, 'ajax')
      spyOn(GERBERA.Auth, 'checkSID')
    })

    beforeEach(function () {
      cookieSpy = spyOn($, 'cookie').and.callFake(function (name) {
        if (name === 'TYPE') return 'db'
        if (name === 'SID') return 'A_MOCK_SID'
        return 'A_MOCK_COOKIE'
      })
    })

    afterEach(function () {
      ajaxSpy.calls.reset()
    })

    it('retrieves the configuration from the server using AJAX', function (done) {
      var isDone = done
      ajaxSpy.and.callFake(function (options) {
        return $.Deferred().resolve(mockConfig).promise()
      })
      GERBERA.App.initialize().then(function () {
        expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface')
        expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
          req_type: 'auth',
          sid: 'A_MOCK_SID',
          action: 'get_config'
        })

        expect(GERBERA.App.serverConfig).toEqual(convertedConfig.config)
        expect(GERBERA.Auth.checkSID).toHaveBeenCalled()
        isDone()
      })
    })

    it('reports an error when the ajax calls fails', function (done) {
      var isDone = done
      spyOn(GERBERA.Updates, 'showMessage')

      ajaxSpy.and.callFake(function () {
        return $.Deferred().reject({responseText: 'Internal Server Error'})
      })

      GERBERA.App.initialize().fail(function () {
        expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Internal Server Error')
        isDone()
      })
    })

    it('stores the TYPE and SID cookies to the document', function (done) {
      var isDone = done
      ajaxSpy.and.callFake(function (options) {
        return $.Deferred().resolve(mockConfig).promise()
      })
      GERBERA.App.initialize().then(function () {
        expect(GERBERA.App.getType()).toEqual('db')
        expect(GERBERA.App.isTypeDb()).toBeTruthy()
        expect(GERBERA.Auth.getSessionId()).toBe('A_MOCK_SID')
        isDone()
      })
    })

    it('defaults the TYPE to `db` when none is set', function (done) {
      var isDone = done
      ajaxSpy.and.callFake(function (options) {
        return $.Deferred().resolve(mockConfig).promise()
      })
      cookieSpy.and.callThrough()

      GERBERA.App.initialize().then(function () {
        expect(GERBERA.App.getType()).toEqual('db')
        expect(GERBERA.App.isTypeDb()).toBeTruthy()
        isDone()
      })
    })

    it('initializes all GERBERA components when logged in', function (done) {
      var isDone = done
      ajaxSpy.and.callFake(function (options) {
        return $.Deferred().resolve(mockConfig).promise()
      })
      cookieSpy.and.callThrough()
      spyOn(GERBERA.Items, 'initialize')
      spyOn(GERBERA.Tree, 'initialize')
      spyOn(GERBERA.Trail, 'initialize')
      spyOn(GERBERA.Autoscan, 'initialize')
      spyOn(GERBERA.Updates, 'initialize')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.App.initialize().then(function () {
        expect(GERBERA.Items.initialize).toHaveBeenCalled()
        expect(GERBERA.Tree.initialize).toHaveBeenCalled()
        expect(GERBERA.Trail.initialize).toHaveBeenCalled()
        expect(GERBERA.Autoscan.initialize).toHaveBeenCalled()
        expect(GERBERA.Updates.initialize).toHaveBeenCalled()
        isDone()
      })
    })
  })
})
