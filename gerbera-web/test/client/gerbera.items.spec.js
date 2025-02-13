/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.items.spec.js - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/
import { GerberaApp } from "../../../web/js/gerbera-app.module";
import { Items, BrowseItemView, SearchItemView, FileItemView } from '../../../web/js/gerbera-items.module';
import { Trail } from "../../../web/js/gerbera-trail.module";
import { Updates } from "../../../web/js/gerbera-updates.module";
import { Auth } from "../../../web/js/gerbera-auth.module";

import mockConfig from './fixtures/config';
import itemsResponse from './fixtures/parent_id-7443-start-0';
import gerberaItems from './fixtures/gerbera-items';
import dataGridData from './fixtures/datagrid-data';
import fileItems from './fixtures/items-for-files';
import editItemResponse from './fixtures/object_id-39479';
import editDisabled from './fixtures/edit_load_disabled';
import itemMock from './fixtures/item';
import externalUrl from './fixtures/external-url';
import containerMock from './fixtures/container';
import treeDataJson from './fixtures/tree-data';

describe('Gerbera Items', () => {
  let lsSpy;
  let viewFactoryData;
  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    viewFactoryData = {
      gerbera: {
        id: 1235
      }
    };
    lsSpy = spyOn(window.localStorage, 'getItem').and.callFake((name) => {
      return;
    });
  });
  afterEach((done) => {
    $("body").on('transitionend', function (event) {
      $('#editModal').remove();
      $('.modal-backdrop').remove();
    });
    fixture.cleanup();
    done();
  });
  beforeAll(() => {
    spyOn(GerberaApp, 'writeLocalStorage').and.callFake(() => { });
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
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      GerberaApp.serverConfig = mockConfig.config;

      await Items.initialize();
      Items.treeItemSelected(viewFactoryData);

      expect(GerberaApp.currentTreeItem).toEqual(viewFactoryData);
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('items');
      expect(ajaxSpy.calls.mostRecent().args[0].data.parent_id).toBe(1235);
      expect(ajaxSpy.calls.mostRecent().args[0].data.start).toBe(0);
      expect(ajaxSpy.calls.mostRecent().args[0].data.count).toBe(50);
    });
  });
  describe('transformItems()', () => {
    beforeEach(() => {
      GerberaApp.serverConfig = mockConfig.config;
    });

    it('converts a gerbera list of items to the widget accepted list', () => {
      const result = Items.transformItems(gerberaItems);
      expect(result).toEqual(dataGridData);
    });
  });
  describe('loadItems()', () => {
    beforeEach(() => {
      GerberaApp.serverConfig = mockConfig.config;
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(GerberaApp, 'gridMode').and.returnValue(0);
      Items.viewFactory(viewFactoryData);
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
      expect($('#datagrid').find('tr').length).toEqual(0);
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
      spyOn(GerberaApp, 'viewItems').and.returnValue(10);

      Items.loadItems(itemsResponse);
      $($('#datagrid nav.grb-pager > ul > li').get(1)).find('a').click();

      expect(Items.retrieveItemsForPage).toHaveBeenCalled();
    });

    it('refreshes the trail using the response items', () => {
      spyOn(Trail, 'makeTrailFromItem');

      Items.loadItems(itemsResponse);

      expect(Trail.makeTrailFromItem).toHaveBeenCalledWith(itemsResponse.items, undefined);
    });
  });
  describe('loadItems() for files', () => {
    beforeEach(() => {
      GerberaApp.serverConfig = mockConfig.config;
      spyOn(GerberaApp, 'getType').and.returnValue('fs');
      Items.viewFactory(viewFactoryData);
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
      expect($('#datagrid').find('tr').length).toEqual(32);
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
      Items.viewFactory(viewFactoryData);
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

    it('calls the server with the `item` details', () => {
      const itemData = {
        item: itemMock
      };
      editModal.editmodal('loadItem', itemData);

      Items.saveItem();
      var data = {
        req_type: 'edit_save',
        object_id: '39479',
        title: 'Test.mp4',
        description: 'A%20description',
        updates: 'check',
        flags: 'Restricted',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
      editModal.remove();
    });

    it('calls the server with the `container` details', () => {
      const itemData = {
        item: containerMock
      };

      editModal.editmodal('loadItem', itemData);

      Items.saveItem();
      var data = {
        req_type: 'edit_save',
        object_id: '1471',
        title: 'container%20title',
        updates: 'check',
        flags: 'Restricted',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
      editModal.remove();
    });

    it('calls the server with the `external_url` details', () => {
      const itemData = {
        item: externalUrl
      };

      editModal.editmodal('loadItem', itemData);

      Items.saveItem();
      var data = {
        req_type: 'edit_save',
        object_id: '1469',
        title: 'title',
        location: 'http%3A%2F%2Flocalhost',
        description: 'description',
        'mime-type': 'video%2Fts',
        protocol: 'http-get',
        updates: 'check',
        flags: 'Restricted',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
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
        return Promise.resolve({ success: true });
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
      editModal.editmodal('addNewItem', { type: 'item', item: item });

      await Items.addObject();
      var data = {
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'item',
        class: 'object.item',
        location: '',
        title: '',
        description: '',
        updates: 'check',
        flags: '',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('allows creation of object under root parent = 0 (database)', async () => {
      const parentId = {
        id: 0
      };
      editModal.editmodal('addNewItem', { type: 'container', item: parentId });

      await Items.addObject();
      var data = {
        req_type: 'add_object',
        parent_id: '0',
        obj_type: 'container',
        class: 'object.container',
        title: '',
        updates: 'check',
        flags: '',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('calls the server with the `container` details', async () => {
      editModal.editmodal('addNewItem', { type: 'container', item: item });

      await Items.addObject();
      var data = {
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'container',
        class: 'object.container',
        title: '',
        updates: 'check',
        flags: '',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('calls the server with the `external_url` details', async () => {
      editModal.editmodal('addNewItem', { type: 'external_url', item: item });

      await Items.addObject();
      var data = {
        req_type: 'add_object',
        parent_id: '9999',
        obj_type: 'external_url',
        class: 'object.item',
        title: '',
        location: '',
        description: '',
        'mime-type': '',
        protocol: 'http-get',
        updates: 'check',
        flags: '',
        resources: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('when successful adds a GERBERA.Updates UI timer to check for updates later', async () => {
      editModal.editmodal('addNewItem', { type: 'item', item: item });
      GerberaApp.currentTreeItem = {};
      await Items.addObject();

      expect(Updates.updateTreeByIds).toHaveBeenCalled();
    });
  });
  describe('retrieveGerberaItems()', () => {
    let ajaxSpy;

    beforeEach(() => {
      GerberaApp.serverConfig = mockConfig.config;
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
      GerberaApp.serverConfig = mockConfig.config;
    });

    it('calls the server for items given correct parameters', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');

      const view = new BrowseItemView(0);
      view.start = 0;
      // view.count = 25;
      await view.retrieveGerberaItems(0);
      var data = {
        req_type: 'items',
        action: 'browse',
        parent_id: 0,
        start: 0,
        count: 50,
        select_it: 0,
        updates: 'check'
      };
      data[Auth.SID] = 'SESSION_ID';
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('calls the server for search given correct parameters', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      const view = new SearchItemView(0);
      view.start = 0;
      view.count = 50;
      view.searchCriteria = 'dc:title contains "music"';
      view.sortCriteria = '-dc:title';
      await view.retrieveGerberaItems(0);
      var data = {
        req_type: 'items',
        action: 'search',
        parent_id: 0,
        start: 0,
        count: 50,
        select_it: 0,
        searchCriteria: 'dc:title contains "music"',
        sortCriteria: '-dc:title',
        searchableContainers: false
      };
      data[Auth.SID] = 'SESSION_ID';
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('calls the server for files given correct parameters', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      const view = new FileItemView(0);
      view.start = 0;
      await view.retrieveGerberaItems(0);
      var data = {
        req_type: 'files',
        action: 'files',
        parent_id: 0,
        start: 0,
        count: 50,
        select_it: 0
      };
      data[Auth.SID] = 'SESSION_ID';
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });
  });
  describe('retrieveItemsForPage()', () => {
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


    it('sends the page number, items per page, and total items to retrieve items', async () => {
      const event = {
        data: {
          gridMode: 0,
          pageNumber: 1,
          itemsPerPage: 50,
          parentId: 1235
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');

      Items.viewFactory(viewFactoryData);
      await Items.retrieveItemsForPage(event);
      var data = {
        req_type: 'items',
        action: 'browse',
        parent_id: 1235,
        start: 0,
        count: 50,
        select_it: 0,
        updates: 'check'
      };
      data[Auth.SID] = 'SESSION_ID';
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
      expect(GerberaApp.currentPage()).toBe(1);
    });

    it('properly sets the start when page number is high', async () => {
      const event = {
        data: {
          gridMode: 0,
          pageNumber: 5,
          itemsPerPage: 50,
          parentId: 1235
        }
      };

      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');

      Items.viewFactory(viewFactoryData);
      await Items.retrieveItemsForPage(event);
      var data = {
        req_type: 'items',
        action: 'browse',
        parent_id: 1235,
        start: 200,
        count: 50,
        select_it: 0,
        updates: 'check'
      };
      data[Auth.SID] = 'SESSION_ID';
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
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
          gridMode: 0,
          itemsPerPage: 50,
          totalMatches: 100,
          parentId: 1235
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      Items.setPage(1);

      Items.viewFactory(viewFactoryData);
      await Items.nextPage(event);
      var data = {
        req_type: 'items',
        action: 'browse',
        parent_id: 1235,
        start: 50,
        count: 50,
        select_it: 0,
        updates: 'check'
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
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
          gridMode: 0,
          itemsPerPage: 50,
          totalMatches: 100,
          parentId: 1235
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      Items.setPage(2);

      Items.viewFactory(viewFactoryData);
      await Items.previousPage(event);
      var data = {
        req_type: 'items',
        action: 'browse',
        parent_id: 1235,
        start: 0,
        count: 50,
        select_it: 0,
        updates: 'check'
      };
      data[Auth.SID] = 'SESSION_ID';
      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
      expect(GerberaApp.currentPage()).toBe(1);
    });
  });
});
