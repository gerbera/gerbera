/* global GERBERA jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures spyOn */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera Items', function () {
  'use strict'
  describe('initialize()', function () {

    var mockConfig

    beforeEach(function () {
      loadJSONFixtures('config.json')
      loadFixtures('index.html')
      mockConfig = getJSONFixture('config.json')
      GERBERA.App.serverConfig = mockConfig.config
    })

    it('loads the # of view items based on default(25) when app config is undefined', function (done) {
      GERBERA.App.serverConfig = {}
      GERBERA.Items.initialize().then(function () {
        expect(GERBERA.Items.viewItems()).toBe(25)
        done()
      })
    })

    it('loads the # of view items based on application config', function (done) {
      GERBERA.Items.initialize().then(function () {
        expect(GERBERA.Items.viewItems()).toBe(50)
        done()
      })
    })
  })

  describe('treeItemSelected()', function () {
    var response

    beforeEach(function () {
      loadJSONFixtures('parent_id-7443-start-0.json')
      response = getJSONFixture('parent_id-7443-start-0.json')
    })

    it('updates the breadcrumb based on the selected item', function (done) {
      var data = {
        gerbera: {
          id: 12345
        }
      }
      var ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        return d.resolve(response)
      })
      spyOn(GERBERA.App, 'getType').and.returnValue('db')
      GERBERA.App.serverConfig = {}

      GERBERA.Items.initialize().then(function () {
        GERBERA.Items.treeItemSelected(data)

        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('items')
        expect(ajaxSpy.calls.mostRecent().args[0].data.parent_id).toBe(12345)
        expect(ajaxSpy.calls.mostRecent().args[0].data.start).toBe(0)
        expect(ajaxSpy.calls.mostRecent().args[0].data.count).toBe(25)
        done()
      })
    })
  })

  describe('transformItems()', function () {
    var gerberaList
    var widgetList

    beforeEach(function () {
      loadJSONFixtures('gerbera-items.json')
      loadJSONFixtures('datagrid-data.json')
      gerberaList = getJSONFixture('gerbera-items.json')
      widgetList = getJSONFixture('datagrid-data.json')
    })

    it('converts a gerbera list of items to the widget accepted list', function () {
      var result = GERBERA.Items.transformItems(gerberaList)
      expect(result).toEqual(widgetList)
    })
  })

  describe('loadItems()', function () {
    var response

    beforeEach(function () {
      loadJSONFixtures('parent_id-7443-start-0.json')
      response = getJSONFixture('parent_id-7443-start-0.json')
      loadFixtures('datagrid.html')
    })

    it('does not load items if response is failure', function () {
      response.success = false
      GERBERA.Items.loadItems(response)
      expect($('#items').find('tr').length).toEqual(0)
      response.success = true
    })

    it('loads the response as items in the datagrid', function () {
      response.success = true
      GERBERA.Items.loadItems(response)
      expect($('#datagrid').find('tr').length).toEqual(12)
      response.success = true
    })
  })
})
