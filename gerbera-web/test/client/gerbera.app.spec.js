/* global GERBERA jasmine it expect spyOn describe beforeEach beforeAll afterEach loadJSONFixtures getJSONFixture */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera UI App', function () {
  describe('initialize()', function () {
    var mockConfig, convertedConfig, ajaxSpy, cookieSpy, uiDisabled

    beforeAll(function () {
      loadJSONFixtures('converted-config.json')
      loadJSONFixtures('config.json')
      loadJSONFixtures('ui-disabled.json')
      mockConfig = getJSONFixture('config.json')
      convertedConfig = getJSONFixture('converted-config.json')
      uiDisabled = getJSONFixture('ui-disabled.json')
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

    it('reports an error when UI is disabled', function (done) {
      var isDone = done
      spyOn(GERBERA.Updates, 'showMessage')
      ajaxSpy.and.callFake(function () {
        return $.Deferred().resolve(uiDisabled).promise()
      })

      GERBERA.App.initialize().fail(function () {
        expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('The UI is disabled in the configuration file. See README.')
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

  describe('disable()', function () {
    it('will disable the menu and clear the content', function () {
      spyOn(GERBERA.Menu, 'disable')
      spyOn(GERBERA.Menu, 'hideLogin')
      spyOn(GERBERA.Menu, 'hideMenu')
      spyOn(GERBERA.Tree, 'destroy')
      spyOn(GERBERA.Trail, 'destroy')
      spyOn(GERBERA.Items, 'destroy')

      GERBERA.App.disable()

      expect(GERBERA.Menu.disable).toHaveBeenCalled()
      expect(GERBERA.Menu.hideLogin).toHaveBeenCalled()
      expect(GERBERA.Menu.hideMenu).toHaveBeenCalled()
      expect(GERBERA.Tree.destroy).toHaveBeenCalled()
      expect(GERBERA.Trail.destroy).toHaveBeenCalled()
      expect(GERBERA.Items.destroy).toHaveBeenCalled()
    })
  });

  describe('error()', function () {
    var uiDisabled

    beforeEach(function () {
      loadJSONFixtures('ui-disabled.json')
      uiDisabled = getJSONFixture('ui-disabled.json')
    })

    it('shows the event as the error message when event is a string', function () {
      spyOn(GERBERA.Updates, 'showMessage')
      var event = 'Error Message'
      GERBERA.App.error(event)

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Error Message')
    })

    it('shows the event as error message when event has responseText', function () {
      spyOn(GERBERA.Updates, 'showMessage')
      var event = {responseText : 'Response Text Error'}
      GERBERA.App.error(event)

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Response Text Error')
    })

    it('shows the event error taken from the response error', function () {
      spyOn(GERBERA.Updates, 'showMessage')
      var event = uiDisabled

      GERBERA.App.error(event)

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('The UI is disabled in the configuration file. See README.')
    })

    it('shows unspecified error when event is invalid', function () {
      spyOn(GERBERA.Updates, 'showMessage')
      var event = undefined

      GERBERA.App.error(event)

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('The system encountered an error')
    })
  });
})
