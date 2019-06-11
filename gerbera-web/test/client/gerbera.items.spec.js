import {GerberaApp} from "../../../web/js/gerbera-app.module";
import {Items} from '../../../web/js/gerbera-items.module';
import {Trail} from "../../../web/js/gerbera-trail.module";
import {Updates} from "../../../web/js/gerbera-updates.module";
import {Auth} from "../../../web/js/gerbera-auth.module";
import mockConfig from './fixtures/config';
import itemsResponse from './fixtures/parent_id-7443-start-0';
import gerberaItems from './fixtures/gerbera-items';
import dataGridData from './fixtures/datagrid-data';
import fileItems from './fixtures/items-for-files';
import editItemResponse from './fixtures/object_id-39479';
import editDisabled from './fixtures/edit_load_disabled';
import itemMock from './fixtures/item';
import activeItemMock from './fixtures/active-item';
import externalUrl from './fixtures/external-url';
import internalUrl from './fixtures/internal-url';
import containerMock from './fixtures/container';
import treeDataJson from './fixtures/tree-data';

describe('Gerbera Items', () => {
  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
  });
  afterEach((done) => {
    $("body").on('transitionend', function(event){
      fixture.cleanup();
      $('#editModal').remove();
      $('.modal-backdrop').remove();
      done();
    });
    fixture.cleanup();
    done();
  });

  describe('initialize()', () => {
   beforeEach(() => {
     GerberaApp.serverConfig = mockConfig.config;
   });

   it('clears the datagrid', async () => {
     await Items.initialize();
     expect($('#datagrid').text()).toBe('');
   });
 });
  describe('treeItemSelected()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve(itemsResponse);
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('updates the breadcrumb based on the selected item', async () => {
      const data = {
        gerbera: {
          id: 12345
        }
      };
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      GerberaApp.serverConfig = {};

      await Items.initialize();
      Items.treeItemSelected(data);

      expect(GerberaApp.currentTreeItem).toEqual(data);
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('items');
      expect(ajaxSpy.calls.mostRecent().args[0].data.parent_id).toBe(12345);
      expect(ajaxSpy.calls.mostRecent().args[0].data.start).toBe(0);
      expect(ajaxSpy.calls.mostRecent().args[0].data.count).toBe(25);
    });
  });
  describe('transformItems()', () => {
    let widgetList;

    beforeEach(() => {
      GerberaApp.serverConfig = {};
    });

    it('converts a gerbera list of items to the widget accepted list', () => {
      const result = Items.transformItems(gerberaItems);
      expect(result).toEqual(dataGridData);
    });
  });
  describe('loadItems()', () => {
    beforeEach(() => {
      GerberaApp.serverConfig = {};
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      $('#tree').tree({
        data: treeDataJson,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });
    });

    it('does not load items if response is failure', () => {
      itemsResponse.success = false;
      Items.loadItems(itemsResponse);
      expect($('#items').find('tr').length).toEqual(0);
      itemsResponse.success = true;
    });

    it('loads the response as items in the datagrid', () => {
      itemsResponse.success = true;
      Items.loadItems(itemsResponse);
      expect($('#datagrid').find('tr').length).toEqual(12);
      itemsResponse.success = true;
    });

    it('loads a pager for items', () => {
      itemsResponse.success = true;
      Items.loadItems(itemsResponse);

      expect($('#datagrid nav.grb-pager').length).toBe(1);

      itemsResponse.success = true;
    });

    it('sets pager click to the callback method', () => {
      itemsResponse.success = true;
      spyOn(Items, 'retrieveItemsForPage');
      spyOn(GerberaApp, 'viewItems').and.returnValue(25);

      Items.loadItems(itemsResponse);
      $($('#datagrid nav.grb-pager > ul > li').get(1)).find('a').click();

      expect(Items.retrieveItemsForPage).toHaveBeenCalled();
    });

    it('refreshes the trail using the response items', () => {
      spyOn(Trail, 'makeTrailFromItem');

      Items.loadItems(itemsResponse);

      expect(Trail.makeTrailFromItem).toHaveBeenCalledWith(itemsResponse.items);
    });
  });
  describe('loadItems() for files', () => {
    let response;

    beforeEach(() => {
      GerberaApp.serverConfig = {};
      spyOn(GerberaApp, 'getType').and.returnValue('fs');
      $('#tree').tree({
        data: treeDataJson,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });
    });

    it('loads file items by transforming them', () => {
      Items.loadItems(fileItems);
      expect($('#datagrid').find('tr').length).toEqual(33);
    });
  });
  describe('deleteItemFromList()', () => {
    let ajaxSpy, event;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
      event = {
        data: { id: 913 }
      };
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to delete the item', () => {
      Items.deleteItemFromList(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('remove');
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913);
      expect(ajaxSpy.calls.mostRecent().args[0].data.all).toBe(0);
    });
  });
  describe('deleteGerberaItem()', () => {
    let ajaxSpy, item;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
      item = { id: 913 };
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to delete the item for single item', () => {
      Items.deleteGerberaItem(item, false);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('remove');
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913);
      expect(ajaxSpy.calls.mostRecent().args[0].data.all).toBe(0);
      expect(ajaxSpy.calls.mostRecent().args[0].data.updates).toBe('check');
    });

    it('calls the server to delete all the items', () => {
      Items.deleteGerberaItem(item, true);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('remove');
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913);
      expect(ajaxSpy.calls.mostRecent().args[0].data.all).toBe(1);
    });
  });
  describe('deleteItemComplete()', () => {
    let response;

    beforeEach(() => {
      spyOn(GerberaApp, 'error');
      spyOn(Items, 'treeItemSelected');
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'updateTreeByIds');
      GerberaApp.currentTreeItem = {};
    });

    it('shows error when the delete fails', () => {
      response = {
        success: false
      };

      Items.deleteComplete(response);

      expect(GerberaApp.error).toHaveBeenCalledWith('Failed to delete item');
    });

    it('does nothing when delete is successful', () => {
      response = {
        success: true
      };

      Items.deleteComplete(response);

      expect(GerberaApp.error).not.toHaveBeenCalled();
    });

    it('refreshes the items when successful', () => {
      response = {
        success: true
      };

      Items.deleteComplete(response);

      expect(Items.treeItemSelected).toHaveBeenCalled();
      expect(Updates.updateTreeByIds).toHaveBeenCalled();
    });
  });
  describe('editItem()', () => {
    let ajaxSpy, event;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to load the item details', () => {
      event = {
        data: { id: 39467 }
      };

      Items.editItem(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('edit_load');
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(39467);
      expect(ajaxSpy.calls.mostRecent().args[0].data.updates).toBe('check');
    });
  });
  describe('loadEditItem()', () => {
    let editObjectType, editTitle,
      editLocation, editClass, editDesc, editMime;

    beforeEach(() => {
      editObjectType = $('#editObjectType');
      editTitle = $('#editTitle');
      editLocation = $('#editLocation');
      editClass = $('#editClass');
      editDesc = $('#editDesc');
      editMime = $('#editMime');
    });

    it('loads the item details into the fields', () => {
      spyOn(Updates, 'updateTreeByIds');

      Items.loadEditItem(editItemResponse);

      expect(editObjectType.val()).toBe('item');
      expect(editTitle.val()).toBe('Test.mp4');
      expect(editLocation.val()).toBe('/folder/location/Test.mp4');
      expect(editClass.val()).toBe('object.item.videoItem');
      expect(editDesc.val()).toBe('A description');
      expect(editMime.val()).toBe('video/mp4');
      expect($('#editModal .modal-title').text()).toBe('Edit Item');
      expect($('#editSave').text()).toBe('Save Item');

      expect(editObjectType.is(':disabled')).toBeTruthy();
      expect(editTitle.is(':disabled')).toBeFalsy();
      expect(editLocation.is(':disabled')).toBeFalsy();
      expect(editClass.is(':disabled')).toBeTruthy();
      expect(editDesc.is(':disabled')).toBeFalsy();
      expect(editMime.is(':disabled')).toBeTruthy();
      expect(Updates.updateTreeByIds).toHaveBeenCalled();
    });

    it('disables the fields based on the server status', () => {
      spyOn(Updates, 'updateTreeByIds');

      Items.loadEditItem(editDisabled);

      expect(editObjectType.is(':disabled')).toBeTruthy();
      expect(editTitle.is(':disabled')).toBeTruthy();
      expect(editLocation.is(':disabled')).toBeTruthy();
      expect(editClass.is(':disabled')).toBeTruthy();
      expect(editDesc.is(':disabled')).toBeTruthy();
      expect(editMime.is(':disabled')).toBeTruthy();
    });
  });
  describe('saveItem()', () => {
    let ajaxSpy, editModal;

    beforeEach(() => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(Items, 'treeItemSelected');
      spyOn(Updates, 'showMessage');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
      editModal = $('#editModal');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server with the item details', () => {
      const itemData = {
        item: itemMock
      };
      editModal.editmodal('loadItem', itemData);

      Items.saveItem();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '39479',
        title: 'Test.mp4',
        description: 'A description',
        updates: 'check'
      });
      editModal.remove();
    });

    it('calls the server with the `container` details', () => {
      const itemData = {
        item: containerMock
      };

      editModal.editmodal('loadItem', itemData);

      Items.saveItem();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1471',
        title: 'container title',
        updates: 'check'
      });
      editModal.remove();
    });

    it('calls the server with the `external_url` details', () => {
      const itemData = {
        item: externalUrl
      };

      editModal.editmodal('loadItem', itemData);

      Items.saveItem();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1469',
        title: 'title',
        location: 'http://localhost',
        description: 'description',
        protocol: 'http-get',
        updates: 'check'
      });
      editModal.remove();
    });

    it('calls the server with the `internal_url` details', () => {
      const itemData = {
        item: internalUrl
      };

      editModal.editmodal('loadItem', itemData);

      Items.saveItem();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'edit_save',
        object_id: '1470',
        title: 'title',
        location: './test',
        description: 'description',
        protocol: 'http-get',
        updates: 'check'
      });
      editModal.remove();
    });

    it('calls the server with the `active_item` details', () => {
      const itemData = {
        item: activeItemMock
      };

      editModal.editmodal('loadItem', itemData);

      Items.saveItem();

      expect(ajaxSpy.calls.count()).toBe(1);
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
      });
      editModal.remove();
    });
  });
  describe('saveItemComplete()', () => {
    let response;

    beforeEach(() => {
      GerberaApp.currentTreeItem = {};
    });

    it('reloads items when successfully saved', () => {
      response = {
        success: true
      };
      spyOn(Updates, 'updateTreeByIds');
      spyOn(Updates, 'showMessage');

      Items.saveItemComplete(response);

      expect(Updates.updateTreeByIds).toHaveBeenCalled();
    });
  });
  describe('addFileItem()', () => {
    let ajaxSpy, event, response;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({})
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to add the item', () => {
      event = {
        data: { id: 913 }
      };

      Items.addFileItem(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('add');
      expect(ajaxSpy.calls.mostRecent().args[0].data.object_id).toBe(913);
    });

    it('shows error when add fails', () => {
      response = {
        success: false
      };
      spyOn(GerberaApp, 'error');

      Items.addFileItemComplete(response);

      expect(GerberaApp.error).toHaveBeenCalledWith('Failed to add item');
    });

    it('shows toast message when add succeeds', () => {
      response = {
        success: true
      };
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'addUiTimer');

      Items.addFileItemComplete(response);

      expect(Updates.showMessage).toHaveBeenCalledWith('Successfully added item', undefined, 'success', 'fa-check');
      expect(Updates.addUiTimer).toHaveBeenCalled();
    });

    it('adds a UI timer to check for updates when successful', () => {
      response = {
        success: true
      };
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'addUiTimer');

      Items.addFileItemComplete(response);

      expect(Updates.addUiTimer).toHaveBeenCalled();
    });
  });
  describe('addVirtualItem()', () => {
    let event;

    it('should show parent id on form when adding new item but no object id', () => {
      event = {
        data: {
          id: 123456
        }
      };

      Items.addVirtualItem(event);

      expect($('#objectIdGroup').css('display')).toBe('none');
      expect($('#parentIdGroup').css('display')).toBe('block');
    });

    it('should set parent id on form when adding new item', () => {
      event = {
        data: {
          id: 123456
        }
      };

      Items.addVirtualItem(event);

      expect($('#addParentId').val()).toBe('123456');
    });

    it('should set form elements to proper values and availability', () => {
      event = {
        data: {
          id: 123456
        }
      };

      Items.addVirtualItem(event);

      expect($('#editModal .modal-title').text()).toBe('Add Item');
    });
  });
  describe('addObject()', () => {
    let ajaxSpy, item, editModal;

    beforeEach(() => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(Items, 'treeItemSelected');
      spyOn(Updates, 'updateTreeByIds');
      spyOn(Updates, 'showMessage');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({success: true});
      });
      item = {
        id: 9999
      };
      editModal = $('#editModal');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server with the item details to add', async () => {
      editModal.editmodal('addNewItem', {type: 'item', item: item});

      await Items.addObject();

      expect(ajaxSpy.calls.count()).toBe(1);
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
      });
    });

    it('allows creation of object under root parent = 0 (database)', async () => {
      const parentId = {
        id: 0
      };
      editModal.editmodal('addNewItem', {type: 'container', item: parentId});

      await Items.addObject();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '0',
        obj_type: 'container',
        class: 'object.container',
        title: '',
        updates: 'check'
      });
    });

    it('calls the server with the `container` details', async () => {
      editModal.editmodal('addNewItem', {type: 'container', item: item});

      await Items.addObject();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'container',
        class: 'object.container',
        title: '',
        updates: 'check'
      });
    });

    it('calls the server with the `external_url` details', async () => {
      editModal.editmodal('addNewItem', {type: 'external_url', item: item});

      await Items.addObject();

      expect(ajaxSpy.calls.count()).toBe(1);
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
      });
    });

    it('calls the server with the `internal_url` details', async () => {
      editModal.editmodal('addNewItem', {type: 'internal_url', item: item});

      await Items.addObject();

      expect(ajaxSpy.calls.count()).toBe(1);
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
      });
    });

    it('calls the server with the `active_item` details', async () => {
      editModal.editmodal('addNewItem', {type: 'active_item', item: item});

      await Items.addObject();

      expect(ajaxSpy.calls.count()).toBe(1);
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
      });
    });

    it('when successful adds a GERBERA.Updates UI timer to check for updates later', async () => {
      editModal.editmodal('addNewItem', {type: 'active_item', item: item});
      GerberaApp.currentTreeItem = {};
      await Items.addObject();

      expect(Updates.updateTreeByIds).toHaveBeenCalled();
    });
  });
  describe('retrieveGerberaItems()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server for items given correct parameters', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');

      await Items.retrieveGerberaItems('db', 0, 0, 25);
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        req_type: 'db',
        sid: 'SESSION_ID',
        parent_id: 0,
        start: 0,
        count: 25,
        updates: 'check'
      });
    });
  });
  describe('retrieveItemsForPage()', () => {
    let ajaxSpy;

    beforeEach(async () => {
      GerberaApp.serverConfig = mockConfig.config;
      GerberaApp.serverConfig = {};

      await Items.initialize();
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });


    it('sends the page number, items per page, and total items to retreive items', async () => {
      const event = {
        data: {
          pageNumber: 1,
          itemsPerPage: 25,
          parentId: 1235
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');

      await Items.retrieveItemsForPage(event);
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        req_type: 'items',
        sid: 'SESSION_ID',
        parent_id: 1235,
        start: 0,
        count: 25,
        updates: 'check'
      });
      expect(GerberaApp.currentPage()).toBe(1);
    });

    it('properly sets the start when page number is high', async () => {
      const event = {
        data: {
          pageNumber: 5,
          itemsPerPage: 25,
          parentId: 1235
        }
      };

      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');

      await Items.retrieveItemsForPage(event);
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        req_type: 'items',
        sid: 'SESSION_ID',
        parent_id: 1235,
        start: 100,
        count: 25,
        updates: 'check'
      });

      expect(GerberaApp.currentPage()).toBe(5);
    });
  });
  describe('nextPage()', () => {
    let ajaxSpy;

    beforeEach(async () => {
      GerberaApp.serverConfig = mockConfig.config;
      await Items.initialize();
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server for the next page based on current page', async () => {
      const event = {
        data: {
          itemsPerPage: 25,
          totalMatches: 100,
          parentId: 1235
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      Items.setPage(1);

      await Items.nextPage(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        req_type: 'items',
        sid: 'SESSION_ID',
        parent_id: 1235,
        start: 25,
        count: 50,
        updates: 'check'
      });
      expect(GerberaApp.currentPage()).toBe(2);
    });
  });
  describe('previousPage()', () => {
    let ajaxSpy;

    beforeEach(async () => {
      GerberaApp.serverConfig = mockConfig.config;
      await Items.initialize();
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server for the previous page based on current page', async () => {
      const event = {
        data: {
          itemsPerPage: 25,
          totalMatches: 100,
          parentId: 1235
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      Items.setPage(2);

      await Items.previousPage(event);
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        req_type: 'items',
        sid: 'SESSION_ID',
        parent_id: 1235,
        start: 0,
        count: 50,
        updates: 'check'
      });
      expect(GerberaApp.currentPage()).toBe(1);
    });
  });
});
