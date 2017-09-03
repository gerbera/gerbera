/* global GERBERA jasmine it expect spyOn describe beforeEach loadJSONFixtures getJSONFixture */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera UI App', function () {
  describe('initialize()', function () {
    var mockConfig, ajaxSpy, cookieSpy

    beforeEach(function () {
      loadJSONFixtures('config.json')
      mockConfig = getJSONFixture('config.json')

      cookieSpy = spyOn($, 'cookie').and.callFake(function (name) {
        if (name === 'TYPE') return 'db'
        if (name === 'SID') return 'A_MOCK_SID'
        return 'A_MOCK_COOKIE'
      })

      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        d.resolve(mockConfig)
        return d.promise()
      })

      spyOn(GERBERA.Auth, 'checkSID')
    })

    it('retrieves the configuration from the server using AJAX', function (done) {
      var isDone = done

      GERBERA.App.initialize().then(function () {
        expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface')
        expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
          req_type: 'auth',
          sid: 'A_MOCK_SID',
          action: 'get_config'
        })

        expect(GERBERA.App.serverConfig).toEqual(mockConfig.config)
        expect(GERBERA.Auth.checkSID).toHaveBeenCalled()
        isDone()
      })
    })

    it('stores the TYPE and SID cookies to the document', function (done) {
      var isDone = done
      GERBERA.App.initialize().then(function () {
        expect(GERBERA.App.getType()).toEqual('db')
        expect(GERBERA.App.isTypeDb()).toBeTruthy()
        expect(GERBERA.Auth.getSessionId()).toBe('A_MOCK_SID')
        isDone()
      })
    })

    it('defaults the TYPE to `db` when none is set', function (done) {
      var isDone = done
      cookieSpy.and.callThrough()

      GERBERA.App.initialize().then(function () {
        expect(GERBERA.App.getType()).toEqual('db')
        expect(GERBERA.App.isTypeDb()).toBeTruthy()
        isDone()
      })
    })
  })
})
