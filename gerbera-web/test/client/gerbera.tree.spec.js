import treeResponse from './fixtures/parent_id-0-select_it-0';
import childTreeResponse from './fixtures/parent_id-7443-select_it-0';
import trailData from './fixtures/trail-data';
import containerAsTree from './fixtures/container-data-astree';

describe('Gerbera Tree', () => {
  'use strict';
  let tree;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    tree = $('#tree');
  });
  afterEach(() => {
    fixture.cleanup();
  });
  describe('initialize()', () => {
    it('loads the tree view', async () => {
      await GERBERA.Tree.initialize();
      expect(tree.html()).toBe('');
    });
  });

  describe('loadTree()', () => {
    let trailConfig, trail;

    beforeEach(() => {
      trailConfig = {};
      trail = $('#trail');
    });

    it('renders the tree with parent and children', () => {
      GERBERA.Tree.loadTree(treeResponse);
      expect($('#tree li').length).toBe(6);
    });

    it('does not render the tree when response is not successful', () => {
      treeResponse.success = false;

      GERBERA.Tree.loadTree(treeResponse);

      expect($('#tree li').length).toBe(0);
      treeResponse.success = true;
    });

    it('clears the breadcrumb and loads the first item on load of a new tree', () => {
      trail.html('').trail({
        data: trailData,
        config: trailConfig
      });

      GERBERA.Tree.loadTree(treeResponse);

      expect(trail.trail('length')).toBe(1);
    });
  });

  describe('transformContainers', () => {
    let result;

    it('transforms containers response suitable for treeview load', () => {
      result = GERBERA.Tree.transformContainers(treeResponse, true);
      expect(result[0].title).toBe('database');
      expect(result).toEqual(containerAsTree);
    });

    it('only creates parent when boolean is passed', () => {
      result = GERBERA.Tree.transformContainers(treeResponse, false);
      expect(result[0].title).toBe('Audio');
    });

    it('creates badges for autoscan containers', () => {
      result = GERBERA.Tree.transformContainers(treeResponse, false);
      expect(result[3].badge.length).toBe(2);
      expect(result[4].badge.length).toBe(2);
    });
  });
  describe('selectType()', () => {
    let ajaxSpy, getUpdatesSpy, getTypeSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax');
      getUpdatesSpy = spyOn(GERBERA.Updates, 'getUpdates');
      getTypeSpy = spyOn(GERBERA.App, 'getType');
      getUpdatesSpy.calls.reset();
      getTypeSpy.calls.reset();
    });

    afterEach(() => {
      getUpdatesSpy.calls.reset();
      getTypeSpy.calls.reset();
      ajaxSpy.and.callThrough();
    });

    it('calls the server for containers when type is db', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      getUpdatesSpy.and.callFake(() => {
        return $.Deferred().resolve().promise();
      });
      getTypeSpy.and.returnValue('db');

      await GERBERA.Tree.selectType('db', 0);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('containers');
    });


    it('calls the server for directories when type is fs', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      getTypeSpy.and.returnValue('fs');

      await GERBERA.Tree.selectType('fs', 0);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('directories');
    });

    it('on failure calls the app error handler', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().reject();
      });
      spyOn(GERBERA.App, 'error');
      getUpdatesSpy.and.callFake(() => {
        return $.Deferred().resolve().promise();
      });
      getTypeSpy.and.returnValue('db');


      try {
        await GERBERA.Tree.selectType('db', 0);
      } catch (err) {
        expect(GERBERA.App.error).toHaveBeenCalled();
      }
    });

    it('when type is db, checks for updates', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      getTypeSpy.and.returnValue('db');

      await GERBERA.Tree.selectType('db', 0);

      expect(GERBERA.Updates.getUpdates).toHaveBeenCalledWith(true);
    });

    it('when type is fs, does not check for updates', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      getTypeSpy.and.returnValue('fs');

      await GERBERA.Tree.selectType('fs', 0);

      expect(GERBERA.Updates.getUpdates).not.toHaveBeenCalled();
    });
  });

  describe('onItemSelected()', () => {
    let onExpandSpy, treeViewConfig, ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('updates the breadcrumb based on the click of a item', () => {
      spyOn(GERBERA.Items, 'treeItemSelected');
      onExpandSpy = jasmine.createSpy('onExpand');
      treeViewConfig = {
        onExpand: onExpandSpy
      };

      GERBERA.Tree.loadTree(treeResponse, treeViewConfig);

      // click an item
      const item = $($('#tree li').get(3));
      item.children('span.folder-title').click();

      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled();
      expect(GERBERA.Items.treeItemSelected.calls.mostRecent().args[0]).toEqual(
        {title: 'Photos', badge: [2], nodes: [], gerbera: {id: 2779, childCount: 2, autoScanMode: 'none', autoScanType: 'none'}}
      );
    });

    it('passes gerbera data to the items plugin', () => {
      spyOn(GERBERA.Items, 'treeItemSelected');
      onExpandSpy = jasmine.createSpy('onExpand');
      treeViewConfig = {
        onExpand: onExpandSpy
      };

      GERBERA.Tree.loadTree(treeResponse, treeViewConfig);

      // click an item
      const item = $($('#tree li').get(3));
      item.children('span.folder-title').click();

      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled();
      expect(GERBERA.Items.treeItemSelected.calls.mostRecent().args[0]).toEqual(
        {title: 'Photos', badge: [2], nodes: [], gerbera: {id: 2779, childCount: 2, autoScanMode: 'none', autoScanType: 'none'}}
      );
    });

    it('changes the color of the selected row', () => {
      spyOn(GERBERA.Items, 'treeItemSelected');
      onExpandSpy = jasmine.createSpy('onExpand');
      treeViewConfig = {
        onExpand: onExpandSpy
      };

      GERBERA.Tree.loadTree(treeResponse, treeViewConfig);

      // click an item
      const item = $($('#tree li').get(3));
      item.children('span.folder-title').click();

      expect(item.hasClass('selected-item')).toBeTruthy();
    });
  });
  describe('onItemExpanded()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls for child items based on the response', () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(childTreeResponse).promise();
      });
      const onSelectionSpy = jasmine.createSpy('onSelection');
      const treeViewConfig = {
        onSelection: onSelectionSpy
      };
      spyOn(GERBERA.App, 'getType').and.returnValue('db');

      GERBERA.Tree.loadTree(treeResponse, treeViewConfig);

      // click an item
      const item = $($('#tree li').get(4));
      item.children('span.folder-title').click();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data.req_type).toBe('containers');
    });
  });
  describe('reloadTreeItem()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to reload tree and generate trail and selects item', async () => {
      GERBERA.Tree.loadTree(treeResponse);

      const treeElement = $('#tree').tree('getElement', 0);

      spyOn(GERBERA.Items, 'treeItemSelected');
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(childTreeResponse).promise();
      });

      await GERBERA.Tree.reloadTreeItem(treeElement);
      expect(ajaxSpy).toHaveBeenCalled();
      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled();
    });
  });
  describe('reloadTreeItemById()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('finds the selected item given only gerbera id and reloads it', async () => {
      GERBERA.Tree.loadTree(treeResponse);

      spyOn(GERBERA.Items, 'treeItemSelected');
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(childTreeResponse).promise();
      });

      await GERBERA.Tree.reloadTreeItemById(0);
      expect(ajaxSpy).toHaveBeenCalled();
      expect(GERBERA.Items.treeItemSelected).toHaveBeenCalled();
    });
  });

  describe('reloadParentTreeItem()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('reloads the parent item', async () => {
      GERBERA.Tree.loadTree(treeResponse);

      spyOn(GERBERA.Items, 'treeItemSelected');
      spyOn(GERBERA.Trail, 'makeTrail');
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(childTreeResponse).promise()
      });

      await GERBERA.Tree.reloadParentTreeItem(8);

      expect(ajaxSpy.calls.mostRecent().args[0].data.parent_id).toBe(0);
    });
  });
  describe('getTreeElementById()', () => {
    it('finds the HTML element given the gerbera id', () => {
      GERBERA.Tree.loadTree(treeResponse);

      const result = GERBERA.Tree.getTreeElementById(8);

      expect(result.length).toBe(1);
      expect(result.data('grb-item')).toEqual({
        title: 'Audio',
        badge: [6],
        nodes: [],
        gerbera: { id: 8, childCount: 6, autoScanMode: 'none', autoScanType: 'none' }
      });
    });
  });
  describe('onAutoscanEdit()', () => {
    let event;
    beforeEach(() => {
      event = {
        data: {
          id: 1111
        }
      };
    });

    it('calls the GERBERA.Autoscan add method with the event', () => {
      spyOn(GERBERA.Autoscan, 'addAutoscan');

      GERBERA.Tree.onAutoscanEdit(event);

      expect(GERBERA.Autoscan.addAutoscan).toHaveBeenCalledWith(event);
    });
  });
});
