/* global GERBERA jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures spyOn */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera Tree', function () {
  'use strict'

  beforeEach(function () {
    loadFixtures('index.html')
    loadJSONFixtures('parent_id-0-select_it-0.json')
    loadJSONFixtures('child-tree-response.json')
    loadJSONFixtures('tree-data.json')
    loadJSONFixtures('trail-data.json')
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
    var response, trailData, trailConfig

    beforeEach(function () {
      response = getJSONFixture('parent_id-0-select_it-0.json')
      trailData = getJSONFixture('trail-data.json')
      trailConfig = {}
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
      $('#trail').html('').trail({
        data: trailData,
        config: trailConfig
      })

      GERBERA.Tree.loadTree(response)

      expect($('#trail').trail('length')).toBe(1)
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

    it('creates badges for autoscan containers', function () {
      loadJSONFixtures('parent_id-0-select_it-0.json')
      loadJSONFixtures('container-data-astree.json')
      var containersResponse = getJSONFixture('parent_id-0-select_it-0.json')

      var result = GERBERA.Tree.transformContainers(containersResponse, false)

      expect(result[3].badge.length).toBe(2)
      expect(result[4].badge.length).toBe(2)
    })
  })

  describe('selectType()', function () {
    var ajaxSpy

    it('calls the server for containers when type is db', function (done) {
      var isDone = done
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })

      GERBERA.Tree.selectType('db', 0).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('containers')
        isDone()
      })
    })

    it('calls the server for containers when type is db', function (done) {
      var isDone = done
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })

      GERBERA.Tree.selectType('fs', 0).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('directories')
        isDone()
      })
    })

    it('on failure calls the app error handler', function (done) {
      var isDone = done
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().reject()
      })
      spyOn(GERBERA.App, 'error')
      spyOn(GERBERA.Updates, 'getUpdates').and.callFake(function () {
        return $.Deferred.resolve().promise()
      })

      GERBERA.Tree.selectType('db', 0).fail(function () {
        expect(GERBERA.App.error).toHaveBeenCalled()
        isDone()
      })
    })

    it('when type is db, checks for updates', function (done) {
      var isDone = done
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      spyOn(GERBERA.Updates, 'getUpdates')
      spyOn(GERBERA.App, 'getType').and.returnValue('db')

      GERBERA.Tree.selectType('db', 0).then(function () {
        expect(GERBERA.Updates.getUpdates).toHaveBeenCalledWith(true)
        isDone()
      })
    })

    it('when type is fs, does not check for updates', function (done) {
      var isDone = done
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      spyOn(GERBERA.Updates, 'getUpdates')
      spyOn(GERBERA.App, 'getType').and.returnValue('fs')

      GERBERA.Tree.selectType('fs', 0).then(function () {
        expect(GERBERA.Updates.getUpdates).not.toHaveBeenCalled()
        isDone()
      })
    })
  })

  describe('onItemSelected()', function () {
    it('updates the breadcrumb based on the click of a item', function () {
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn($, 'ajax')
      var onExpandSpy = jasmine.createSpy('onExpand')
      var treeViewConfig = {
        onExpand: onExpandSpy
      }

      GERBERA.Tree.loadTree(response, treeViewConfig)

      // click an item
      var item = $($('#tree li').get(3))
      item.children('span.folder-title').click()

      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
      expect(GERBERA.Items.treeItemSelected.calls.mostRecent().args[0]).toEqual(
        {title: 'Photos', badge: [2], nodes: [], gerbera: {id: 2779, childCount: 2, autoScanMode: 'none', autoScanType: 'none'}}
      )
    })

    it('passes gerbera data to the items plugin', function () {
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn($, 'ajax')
      var onExpandSpy = jasmine.createSpy('onExpand')
      var treeViewConfig = {
        onExpand: onExpandSpy
      }

      GERBERA.Tree.loadTree(response, treeViewConfig)

      // click an item
      var item = $($('#tree li').get(3))
      item.children('span.folder-title').click()

      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
      expect(GERBERA.Items.treeItemSelected.calls.mostRecent().args[0]).toEqual(
        {title: 'Photos', badge: [2], nodes: [], gerbera: {id: 2779, childCount: 2, autoScanMode: 'none', autoScanType: 'none'}}
      )
    })

    it('changes the color of the selected row', function () {
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn($, 'ajax')
      var onExpandSpy = jasmine.createSpy('onExpand')
      var treeViewConfig = {
        onExpand: onExpandSpy
      }

      GERBERA.Tree.loadTree(response, treeViewConfig)

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
        return $.Deferred().resolve(childResponse).promise()
      })
      var onSelectionSpy = jasmine.createSpy('onSelection')
      var treeViewConfig = {
        onSelection: onSelectionSpy
      }

      GERBERA.Tree.loadTree(parentResponse, treeViewConfig)

      // click an item
      var item = $($('#tree li').get(4))
      item.children('span.folder-title').click()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('containers')
    })
  })

  describe('reloadTreeItem()', function () {
    var response, childResponse, ajaxSpy
    beforeEach(function () {
      response = getJSONFixture('parent_id-0-select_it-0.json')
      childResponse = getJSONFixture('parent_id-7443-select_it-0.json')
    })

    it('calls the server to reload tree and generate trail and selects item', function (done) {
      GERBERA.Tree.loadTree(response)

      var treeElement = $('#tree').tree('getElement', 0)

      spyOn(GERBERA.Items, 'treeItemSelected')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve(childResponse).promise()
      })

      GERBERA.Tree.reloadTreeItem(treeElement).then(function () {
        expect(ajaxSpy).toHaveBeenCalled()
        expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
        done()
      })
    })
  })

  describe('reloadTreeItemById()', function () {
    var response, childResponse, ajaxSpy
    beforeEach(function () {
      response = getJSONFixture('parent_id-0-select_it-0.json')
      childResponse = getJSONFixture('parent_id-7443-select_it-0.json')
    })

    it('finds the selected item given only gerbera id and reloads it', function (done) {
      GERBERA.Tree.loadTree(response)

      spyOn(GERBERA.Items, 'treeItemSelected')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve(childResponse).promise()
      })

      GERBERA.Tree.reloadTreeItemById(0).then(function () {
        expect(ajaxSpy).toHaveBeenCalled()
        expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
        done()
      })
    })
  })

  describe('reloadParentTreeItem()', function () {
    var response, childResponse, ajaxSpy
    beforeEach(function () {
      response = getJSONFixture('parent_id-0-select_it-0.json')
      childResponse = getJSONFixture('parent_id-7443-select_it-0.json')
    })

    it('reloads the parent item', function (done) {
      GERBERA.Tree.loadTree(response)

      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn(GERBERA.Trail, 'makeTrail')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve(childResponse).promise()
      })

      GERBERA.Tree.reloadParentTreeItem(8).then(function () {
        expect(ajaxSpy.calls.mostRecent().args[0].data.parent_id).toBe(0)
        done()
      })
    })
  })

  describe('getTreeElementById()', function () {
    var response
    beforeEach(function () {
      response = getJSONFixture('parent_id-0-select_it-0.json')
    })

    it('finds the HTML element given the gerbera id', function () {
      GERBERA.Tree.loadTree(response)

      var result = GERBERA.Tree.getTreeElementById(8)

      expect(result.length).toBe(1)
      expect(result.data('grb-item')).toEqual({
        title: 'Audio',
        badge: [6],
        nodes: [],
        gerbera: { id: 8, childCount: 6, autoScanMode: 'none', autoScanType: 'none' }
      })
    })
  })

  describe('onAutoscanEdit()', function () {
    var event
    beforeEach(function () {
      event = {
        data: {
          id: 1111
        }
      }
    })

    it('calls the GERBERA.Autoscan add method with the event', function () {
      spyOn(GERBERA.Autoscan, 'addAutoscan')

      GERBERA.Tree.onAutoscanEdit(event)

      expect(GERBERA.Autoscan.addAutoscan).toHaveBeenCalledWith(event)
    })
  })
})
