/* global GERBERA jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures spyOn */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera Tree', function () {
  'use strict'

  beforeEach(function () {
    loadFixtures('index.html')
    loadJSONFixtures('parent_id-0-select_it-0.json')
    loadJSONFixtures('tree-data.json')
  })

  describe('initialize()', function () {
    it('loads the tree view', function (done) {
      GERBERA.Tree.initialize().then(function () {
        expect($('#tree').html()).toBe('')
        done()
      })
    })
  })

  describe('loadTree()', function () {
    var response

    beforeEach(function () {
      response = getJSONFixture('parent_id-0-select_it-0.json')
    })

    it('renders the tree with parent and children', function () {
      GERBERA.Tree.loadTree(response)
      expect($('#tree li').length).toBe(6)
    })

    it('does not render the tree when response is not successful', function () {
      response.success = false

      GERBERA.Tree.loadTree(response)

      expect($('#tree li').length).toBe(0)
      response.success = true
    })

    it('clears the breadcrumb and loads the first item on load of a new tree', function () {
      $('#item-breadcrumb').html('').append('<li>test</li>')

      GERBERA.Tree.loadTree(response)

      expect($('#item-breadcrumb').children('li').length).toBe(1)
    })
  })

  describe('transformContainers', function () {
    it('transforms containers response suitable for treeview load', function () {
      loadJSONFixtures('parent_id-0-select_it-0.json')
      loadJSONFixtures('container-data-astree.json')
      var containersResponse = getJSONFixture('parent_id-0-select_it-0.json')
      var treeData = getJSONFixture('container-data-astree.json')
      var result = GERBERA.Tree.transformContainers(containersResponse, true)
      expect(result[0].title).toBe('database')
      expect(result).toEqual(treeData)
    })

    it('only creates parent when boolean is passed', function () {
      loadJSONFixtures('parent_id-0-select_it-0.json')
      loadJSONFixtures('container-data-astree.json')
      var containersResponse = getJSONFixture('parent_id-0-select_it-0.json')
      var result = GERBERA.Tree.transformContainers(containersResponse, false)
      expect(result[0].title).toBe('Audio')
    })
  })

  describe('selectType()', function () {
    var ajaxSpy

    it('calls the server for containers when type is db', function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        return d.promise({})
      })

      GERBERA.Tree.selectType('db', 0)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('containers')
    })

    it('calls the server for containers when type is db', function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        return d.promise({})
      })

      GERBERA.Tree.selectType('fs', 0)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('directories')
    })

    it('on failure calls the app error handler', function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        return d.reject()
      })
      spyOn(GERBERA.App, 'error')

      GERBERA.Tree.selectType('db', 0)

      expect(GERBERA.App.error).toHaveBeenCalled()
    })
  })

  describe('onItemSelected()', function () {
    it('updates the breadcrumb based on the click of a item', function () {
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn($, 'ajax')

      GERBERA.Tree.loadTree(response)

      // click an item
      var item = $($('#tree li').get(3))
      item.children('span.folder-title').click()

      expect($('#item-breadcrumb').children('li').length).toBe(2)
      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
      expect(GERBERA.Items.treeItemSelected.calls.mostRecent().args[0]).toEqual(
        {title: 'Photos', badge: [2], nodes: [], gerbera: {id: 2779, childCount: 2}}
      )
    })

    it('passes gerbera data to the items plugin', function () {
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn($, 'ajax')

      GERBERA.Tree.loadTree(response)

      // click an item
      var item = $($('#tree li').get(3))
      item.children('span.folder-title').click()

      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
      expect(GERBERA.Items.treeItemSelected.calls.mostRecent().args[0]).toEqual(
        {title: 'Photos', badge: [2], nodes: [], gerbera: {id: 2779, childCount: 2}}
      )
    })

    it('changes the color of the selected row', function () {
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn($, 'ajax')

      GERBERA.Tree.loadTree(response)

      // click an item
      var item = $($('#tree li').get(3))
      item.children('span.folder-title').click()

      expect(item.hasClass('selected-item')).toBeTruthy()
    })
  })

  describe('onItemExpanded()', function () {
    var ajaxSpy
    it('calls for child items based on the response', function () {
      GERBERA.App.setType('db')
      var parentResponse = getJSONFixture('parent_id-0-select_it-0.json')
      var childResponse = getJSONFixture('parent_id-7443-select_it-0.json')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        var d = $.Deferred()
        d.resolve(childResponse)
        return d.promise()
      })

      GERBERA.Tree.loadTree(parentResponse)

      // click an item
      var item = $($('#tree li').get(4))
      item.children('span.folder-icon').click()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('containers')
    })
  })
})
