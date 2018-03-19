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
        return $.Deferred().resolve(response).promise()
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
      GERBERA.App.serverConfig = {}
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

    it('loads a pager for items', function () {
      response.success = true
      GERBERA.Items.loadItems(response)

      expect($('#datagrid nav.grb-pager').length).toBe(1)

      response.success = true
    })

    it('sets pager click to the callback method', function () {
      response.success = true
      spyOn(GERBERA.Items, 'retrieveItemsForPage')
      spyOn(GERBERA.Items, 'viewItems').and.returnValue(25)

      GERBERA.Items.loadItems(response)
      $($('#datagrid nav.grb-pager > ul > li').get(1)).find('a').click()

      expect(GERBERA.Items.retrieveItemsForPage).toHaveBeenCalled()
    })

    it('stores the currently selected tree item', function () {
      var treeItem = {
        gerbera: {
          id: 12345
        }
      }

      response.success = true
      GERBERA.Items.loadItems(response, treeItem)

      expect(GERBERA.Items.currentTreeItem).toBeDefined()
      expect(GERBERA.Items.currentTreeItem).toEqual(treeItem)
    })

    it('refreshes the trail using the response items', function () {
      spyOn(GERBERA.Trail, 'makeTrailFromItem')

      GERBERA.Items.loadItems(response)

      expect(GERBERA.Trail.makeTrailFromItem).toHaveBeenCalledWith(response.items)
    })
  })

  describe('loadItems() for files', function () {
    var response

    beforeEach(function () {
      loadJSONFixtures('items-for-files.json')
      response = getJSONFixture('items-for-files.json')
      loadFixtures('datagrid.html')
      GERBERA.App.serverConfig = {}
      spyOn(GERBERA.App, 'getType').and.returnValue('fs')
    })

    it('loads file items by transforming them', function () {
      GERBERA.Items.loadItems(response)
      expect($('#datagrid').find('tr').length).toEqual(33)
    })
  })

  describe('deleteItemFromList()', function () {
    var ajaxSpy, event

    beforeEach(function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      event = {
        data: { id: 913 }
      }
    })

    it('calls the server to delete the item', function () {
      GERBERA.Items.deleteItemFromList(event)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('remove')
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913)
      expect(ajaxSpy.calls.mostRecent().args[0].data.all).toBe(0)
    })
  })

  describe('deleteGerberaItem()', function () {
    var ajaxSpy, item

    beforeEach(function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      item = { id: 913 }
    })

    it('calls the server to delete the item for single item', function () {
      GERBERA.Items.deleteGerberaItem(item, false)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('remove')
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913)
      expect(ajaxSpy.calls.mostRecent().args[0].data.all).toBe(0)
      expect(ajaxSpy.calls.mostRecent().args[0].data.updates).toBe('check')
    })

    it('calls the server to delete all the items', function () {
      GERBERA.Items.deleteGerberaItem(item, true)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('remove')
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913)
      expect(ajaxSpy.calls.mostRecent().args[0].data.all).toBe(1)
    })
  })

  describe('deleteItemComplete()', function () {
    var response

    beforeEach(function () {
      spyOn(GERBERA.App, 'error')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn(GERBERA.Updates, 'showMessage')
      spyOn(GERBERA.Updates, 'updateTreeByIds')
    })

    it('shows error when the delete fails', function () {
      response = {
        success: false
      }

      GERBERA.Items.deleteComplete(response)

      expect(GERBERA.App.error).toHaveBeenCalledWith('Failed to delete item')
    })

    it('does nothing when delete is successful', function () {
      response = {
        success: true
      }

      GERBERA.Items.deleteComplete(response)

      expect(GERBERA.App.error).not.toHaveBeenCalled()
    })

    it('refreshes the items when successful', function () {
      response = {
        success: true
      }

      GERBERA.Items.deleteComplete(response)

      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled()
      expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled()
    })
  })

  describe('editItem()', function () {
    var ajaxSpy, event

    it('calls the server to load the item details', function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      event = {
        data: { id: 39467 }
      }

      GERBERA.Items.editItem(event)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('edit_load')
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(39467)
      expect(ajaxSpy.calls.mostRecent().args[0].data.updates).toBe('check')
    })
  })

  describe('loadEditItem()', function () {
    var response

    beforeEach(function () {
      loadJSONFixtures('object_id-39479.json')
      loadJSONFixtures('edit_load_disabled.json')
      loadFixtures('index.html')
    })

    it('loads the item details into the fields', function () {
      response = getJSONFixture('object_id-39479.json')
      spyOn(GERBERA.Updates, 'updateTreeByIds')

      GERBERA.Items.loadEditItem(response)

      expect($('#editObjectType').val()).toBe('item')
      expect($('#editTitle').val()).toBe('Test.mp4')
      expect($('#editLocation').val()).toBe('/folder/location/Test.mp4')
      expect($('#editClass').val()).toBe('object.item.videoItem')
      expect($('#editDesc').val()).toBe('A description')
      expect($('#editMime').val()).toBe('video/mp4')
      expect($('#editModal .modal-title').text()).toBe('Edit Item')
      expect($('#editSave').text()).toBe('Save Item')

      expect($('#editObjectType').is(':disabled')).toBeTruthy()
      expect($('#editTitle').is(':disabled')).toBeFalsy()
      expect($('#editLocation').is(':disabled')).toBeFalsy()
      expect($('#editClass').is(':disabled')).toBeTruthy()
      expect($('#editDesc').is(':disabled')).toBeFalsy()
      expect($('#editMime').is(':disabled')).toBeTruthy()
      expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled()
    })

    it('disables the fields based on the server status', function () {
      response = getJSONFixture('edit_load_disabled.json')
      spyOn(GERBERA.Updates, 'updateTreeByIds')

      GERBERA.Items.loadEditItem(response)

      expect($('#editObjectType').is(':disabled')).toBeTruthy()
      expect($('#editTitle').is(':disabled')).toBeTruthy()
      expect($('#editLocation').is(':disabled')).toBeTruthy()
      expect($('#editClass').is(':disabled')).toBeTruthy()
      expect($('#editDesc').is(':disabled')).toBeTruthy()
      expect($('#editMime').is(':disabled')).toBeTruthy()
    })
  })

  describe('saveItem()', function () {
    var ajaxSpy

    beforeEach(function () {
      loadFixtures('index.html')
      loadJSONFixtures('item.json')
      loadJSONFixtures('active-item.json')
      loadJSONFixtures('external-url.json')
      loadJSONFixtures('internal-url.json')
      loadJSONFixtures('container.json')
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn(GERBERA.Updates, 'showMessage')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
    })

    it('calls the server with the item details', function () {
      var itemData = { item: getJSONFixture('item.json') }
      $('#editModal').editmodal('loadItem', itemData)

      GERBERA.Items.saveItem()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '39479',
        title: 'Test.mp4',
        description: 'A description',
        updates: 'check'
      })
    })

    it('calls the server with the `container` details', function () {
      var itemData = {
        item: getJSONFixture('container.json')
      }

      $('#editModal').editmodal('loadItem', itemData)

      GERBERA.Items.saveItem()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1471',
        title: 'container title',
        updates: 'check'
      })
    })

    it('calls the server with the `external_url` details', function () {
      var itemData = {
        item: getJSONFixture('external-url.json')
      }

      $('#editModal').editmodal('loadItem', itemData)

      GERBERA.Items.saveItem()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1469',
        title: 'title',
        location: 'http://localhost',
        description: 'description',
        protocol: 'http-get',
        updates: 'check'
      })
    })

    it('calls the server with the `internal_url` details', function () {
      var itemData = {
        item: getJSONFixture('internal-url.json')
      }

      $('#editModal').editmodal('loadItem', itemData)

      GERBERA.Items.saveItem()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1470',
        title: 'title',
        location: './test',
        description: 'description',
        protocol: 'http-get',
        updates: 'check'
      })
    })

    it('calls the server with the `active_item` details', function () {
      var itemData = {
        item: getJSONFixture('active-item.json')
      }

      $('#editModal').editmodal('loadItem', itemData)

      GERBERA.Items.saveItem()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1306',
        title: 'Test Active Item',
        location: '/home/test.txt',
        description: 'test',
        'mime-type': 'text/plain',
        action: '/home/echoText.sh',
        state: 'test-state',
        updates: 'check'
      })
    })
  })

  describe('saveItemComplete()', function () {
    var response

    beforeEach(function () {
      loadFixtures('index.html')
    })

    it('reloads items when successfully saved', function () {
      response = {
        success: true
      }
      spyOn(GERBERA.Updates, 'updateTreeByIds')
      spyOn(GERBERA.Updates, 'showMessage')

      GERBERA.Items.saveItemComplete(response)

      expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled()
    })
  })

  describe('addFileItem()', function () {
    var ajaxSpy, event, response

    it('calls the server to add the item', function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      event = {
        data: { id: 913 }
      }

      GERBERA.Items.addFileItem(event)

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('add')
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913)
    })

    it('shows error when add fails', function () {
      response = {
        success: false
      }
      spyOn(GERBERA.App, 'error')

      GERBERA.Items.addFileItemComplete(response)

      expect(GERBERA.App.error).toHaveBeenCalledWith('Failed to add item')
    })

    it('shows toast message when add succeeds', function () {
      response = {
        success: true
      }
      spyOn(GERBERA.Updates, 'showMessage')

      GERBERA.Items.addFileItemComplete(response)

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Successfully added item')
    })

    it('adds a UI timer to check for updates when successful', function () {
      response = {
        success: true
      }
      spyOn(GERBERA.Updates, 'showMessage')
      spyOn(GERBERA.Updates, 'addUiTimer')

      GERBERA.Items.addFileItemComplete(response)

      expect(GERBERA.Updates.addUiTimer).toHaveBeenCalled()
    })
  })

  describe('addVirtualItem()', function () {
    var event

    beforeEach(function () {
      loadFixtures('index.html')
    })

    it('should show parent id on form when adding new item but no object id', function () {
      event = {
        data: {
          id: 123456
        }
      }

      GERBERA.Items.addVirtualItem(event)

      expect($('#objectIdGroup').css('display')).toBe('none')
      expect($('#parentIdGroup').css('display')).toBe('block')
    })

    it('should set parent id on form when adding new item', function () {
      event = {
        data: {
          id: 123456
        }
      }

      GERBERA.Items.addVirtualItem(event)

      expect($('#addParentId').val()).toBe('123456')
    })

    it('should set form elements to proper values and availability', function () {
      event = {
        data: {
          id: 123456
        }
      }

      GERBERA.Items.addVirtualItem(event)

      expect($('#editModal .modal-title').text()).toBe('Add Item')
    })
  })

  describe('addObject()', function () {
    var ajaxSpy
    var item

    beforeEach(function () {
      loadFixtures('index.html')
      loadJSONFixtures('item.json')
      loadJSONFixtures('active-item.json')
      loadJSONFixtures('external-url.json')
      loadJSONFixtures('internal-url.json')
      loadJSONFixtures('container.json')
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')
      spyOn(GERBERA.Items, 'treeItemSelected')
      spyOn(GERBERA.Updates, 'updateTreeByIds')
      spyOn(GERBERA.Updates, 'showMessage')
      ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({success: true}).promise()
      })
      item = {
        id: 9999
      }
    })

    it('calls the server with the item details to add', function () {
      $('#editModal').editmodal('addNewItem', {type: 'item', item: item})

      GERBERA.Items.addObject()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'item',
        class: 'object.item',
        location: '',
        title: '',
        description: '',
        updates: 'check'
      })
    })

    it('allows creation of object under root parent = 0 (database)', function () {
      var parentId = {
        id: 0
      }
      $('#editModal').editmodal('addNewItem', {type: 'container', item: parentId})

      GERBERA.Items.addObject()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '0',
        obj_type: 'container',
        class: 'object.container',
        title: '',
        updates: 'check'
      })
    })

    it('calls the server with the `container` details', function () {
      $('#editModal').editmodal('addNewItem', {type: 'container', item: item})

      GERBERA.Items.addObject()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'container',
        class: 'object.container',
        title: '',
        updates: 'check'
      })
    })

    it('calls the server with the `external_url` details', function () {
      $('#editModal').editmodal('addNewItem', {type: 'external_url', item: item})

      GERBERA.Items.addObject()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'external_url',
        class: 'object.item',
        title: '',
        location: '',
        description: '',
        protocol: 'http-get',
        updates: 'check'
      })
    })

    it('calls the server with the `internal_url` details', function () {
      $('#editModal').editmodal('addNewItem', {type: 'internal_url', item: item})

      GERBERA.Items.addObject()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'internal_url',
        class: 'object.item',
        title: '',
        location: '',
        description: '',
        protocol: '',
        updates: 'check'
      })
    })

    it('calls the server with the `active_item` details', function () {
      $('#editModal').editmodal('addNewItem', {type: 'active_item', item: item})

      GERBERA.Items.addObject()

      expect(ajaxSpy.calls.count()).toBe(1)
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'active_item',
        class: 'object.item.activeItem',
        title: '',
        location: '',
        description: '',
        'mime-type': '',
        action: '',
        state: '',
        updates: 'check'
      })
    })

    it('when successful adds a GERBERA.Updates UI timer to check for updates later', function () {
      $('#editModal').editmodal('addNewItem', {type: 'active_item', item: item})
      GERBERA.Items.currentTreeItem = {}
      GERBERA.Items.addObject()

      expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled()
    })
  })

  describe('retrieveGerberaItems()', function () {
    it('calls the server for items given correct parameters', function (done) {
      var isDone = done
      var ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })

      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')

      GERBERA.Items.retrieveGerberaItems('db', 0, 0, 25).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
          req_type: 'db',
          sid: 'SESSION_ID',
          parent_id: 0,
          start: 0,
          count: 25,
          updates: 'check'
        })
        isDone()
      })
    })
  })

  describe('retrieveItemsForPage()', function () {
    it('sends the page number, items per page, and total items to retreive items', function (done) {
      var isDone = done
      var ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      var event = {
        data: {
          pageNumber: 1,
          itemsPerPage: 25,
          parentId: 1235
        }
      }
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')

      GERBERA.Items.retrieveItemsForPage(event).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
          req_type: 'items',
          sid: 'SESSION_ID',
          parent_id: 1235,
          start: 0,
          count: 25,
          updates: 'check'
        })
        expect(GERBERA.Items.currentPage()).toBe(1)
        isDone()
      })
    })

    it('properly sets the start when page number is high', function (done) {
      var isDone = done
      var ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      var event = {
        data: {
          pageNumber: 5,
          itemsPerPage: 25,
          parentId: 1235
        }
      }

      spyOn(GERBERA.Items, 'setPage')
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')

      GERBERA.Items.retrieveItemsForPage(event).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
          req_type: 'items',
          sid: 'SESSION_ID',
          parent_id: 1235,
          start: 100,
          count: 25,
          updates: 'check'
        })

        expect(GERBERA.Items.currentPage()).toBe(5)
        isDone()
      })
    })
  })

  describe('nextPage()', function () {
    it('calls the server for the next page based on current page', function (done) {
      var isDone = done
      var ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      var event = {
        data: {
          itemsPerPage: 25,
          totalMatches: 100,
          parentId: 1235
        }
      }
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')
      spyOn(GERBERA.App, 'getType').and.returnValue('db')
      GERBERA.Items.setPage(1)

      GERBERA.Items.nextPage(event).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
          req_type: 'items',
          sid: 'SESSION_ID',
          parent_id: 1235,
          start: 25,
          count: 25,
          updates: 'check'
        })
        expect(GERBERA.Items.currentPage()).toBe(2)
        isDone()
      })
    })
  })

  describe('previousPage()', function () {
    it('calls the server for the previous page based on current page', function (done) {
      var isDone = done
      var ajaxSpy = spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve({}).promise()
      })
      var event = {
        data: {
          itemsPerPage: 25,
          totalMatches: 100,
          parentId: 1235
        }
      }
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID')
      spyOn(GERBERA.App, 'getType').and.returnValue('db')
      GERBERA.Items.setPage(2)

      GERBERA.Items.previousPage(event).then(function () {
        expect(ajaxSpy.calls.count()).toBe(1)
        expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
          req_type: 'items',
          sid: 'SESSION_ID',
          parent_id: 1235,
          start: 0,
          count: 25,
          updates: 'check'
        })
        expect(GERBERA.Items.currentPage()).toBe(1)
        isDone()
      })
    })
  })
})
