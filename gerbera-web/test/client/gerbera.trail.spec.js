import {Autoscan} from "../../../web/js/gerbera-autoscan.module";
import {GerberaApp} from "../../../web/js/gerbera-app.module";
import {Items} from "../../../web/js/gerbera-items.module";
import {Trail} from "../../../web/js/gerbera-trail.module";
import {Tree} from "../../../web/js/gerbera-tree.module";
import {Updates} from "../../../web/js/gerbera-updates.module";
import treeResponse from './fixtures/parent_id-0-select_it-0';

describe('Gerbera Trail', () => {
  describe('initialize()', () => {
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
    });
    afterEach(() => {
      fixture.cleanup();
    });
    it('clears any data associated with the trail', async () => {
      const trail = $('#trail');

      await Trail.initialize();

      expect(trail.children().length).toBe(0)
    });
  });
  describe('makeTrail()', () => {
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
    });
    afterEach(() => {
      fixture.cleanup();
    });
    it('creates a trail of elements given a selected item in the tree', () => {
      const treeElement = $('#first-child-one-one');
      const trail = $('#trail');

      Trail.makeTrail(treeElement);

      expect(trail.hasClass('grb-trail')).toBeTruthy();
      expect($('#trail ol.breadcrumb').length).toBe(1);
    });
  });
  describe('destroy()', () => {
    let treeElement, trail;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
      treeElement = $('#first-child-one-one');
      trail = $('#trail');
      Trail.makeTrail(treeElement);
    });
    afterEach(() => {
      fixture.cleanup();
    });

    it('removes contents of the trail', () => {
      Trail.destroy();
      expect(trail.children().length).toBe(0);
    });
  });
  describe('gatherTrail()', () => {
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
    });
    afterEach(() => {
      fixture.cleanup();
    });
    it('collects trail data from the tree', () => {
      fixture.load('trail.html');
      const treeElement = $('#first-child-one-one');
      const expected = [
        { id: 111, text: 'first' },
        { id: 1111, text: 'first.child.one' },
        { id: 11111, text: 'first.child.one.one' }
      ];

      const result = Trail.gatherTrail(treeElement);

      expect(result).toEqual(expected);
    });

    it('collects trail data from the tree that has non-numeric IDs', () => {
      fixture.load('trail-fs.html');
      const treeElement = $('#first-child-one-one');
      const expected = [
        { id: 'aaa', text: 'first' },
        { id: 'aaaa', text: 'first.child.one' },
        { id: 'aaaaa', text: 'first.child.one.one' }
      ];

      const result = Trail.gatherTrail(treeElement);

      expect(result).toEqual(expected);
    });
  });
  describe('deleteItem()', () => {
    let deleteResponse, event, appErrorSpy;
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
      deleteResponse = { success: true };
      event = {data: { id: 8 }};

    });
    afterEach(() => {
      appErrorSpy.calls.reset();
      fixture.cleanup();
    });
    it('calls the server to delete the gerbera item', async () => {
      appErrorSpy = spyOn(GerberaApp, 'error');
      Tree.loadTree(treeResponse);
      spyOn(Items, 'deleteGerberaItem').and.callFake(() => {
        return Promise.resolve(deleteResponse);
      });
      spyOn(Updates, 'updateTreeByIds');
      spyOn(Updates, 'showMessage');

      await Trail.deleteItem(event);

      expect(appErrorSpy).not.toHaveBeenCalled();
      expect(Items.deleteGerberaItem).toHaveBeenCalledWith(event.data);
      expect(Updates.updateTreeByIds).toHaveBeenCalled();
      expect(Updates.showMessage).toHaveBeenCalledWith('Successfully removed item', undefined, 'success', 'fa-check');
    });

    it('reports an error if the delete fails', async () => {
      appErrorSpy = spyOn(GerberaApp, 'error');
      Tree.loadTree(treeResponse);
      deleteResponse = { success: false };
      spyOn(Items, 'deleteGerberaItem').and.callFake(() => {
        return Promise.resolve(deleteResponse);
      });
      spyOn(Updates, 'updateTreeByIds');

      await Trail.deleteItem(event);

      expect(appErrorSpy).toHaveBeenCalledWith('Failed to remove item');
      expect(Updates.updateTreeByIds).not.toHaveBeenCalled();
    });
  });
  describe('deleteAllItems()', () => {
    let deleteResponse, event, appErrorSpy;
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
      deleteResponse = {success: true};
      event = {data: {id: 8}};
      appErrorSpy = spyOn(GerberaApp, 'error');
    });
    afterEach(() => {
      appErrorSpy.calls.reset();
      fixture.cleanup();
    });
    it('calls the server to delete the gerbera item', async () => {
      Tree.loadTree(treeResponse);
      spyOn(Items, 'deleteGerberaItem').and.callFake(() => {
        return Promise.resolve(deleteResponse);
      });
      spyOn(Updates, 'updateTreeByIds');
      spyOn(Updates, 'showMessage');

      await Trail.deleteAllItems(event);
      expect(appErrorSpy).not.toHaveBeenCalled();
      expect(Items.deleteGerberaItem).toHaveBeenCalledWith(event.data, true);
      expect(Updates.updateTreeByIds).toHaveBeenCalled();
      expect(Updates.showMessage).toHaveBeenCalledWith('Successfully removed all items', undefined, 'success', 'fa-check');
    });
  });
  describe('makeTrailFromItem()', () => {
    let items, trail;
    beforeEach(() => {
      items = {
        'parent_id': 467,
        'location': '/Video',
        'virtual': true,
        'start': 0,
        'total_matches': 0,
        'autoscan_mode': 'none',
        'autoscan_type': 'none',
        'protect_container': false,
        'protect_items': false,
        'item': []
      };
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
      trail = $('#trail');
    });

    afterEach(() => {
      fixture.cleanup();
    });

    it('uses and item response to make the trail', () => {
      const treeElement = $('#first-child-one-one');
      spyOn(Tree, 'getTreeElementById').and.returnValue(treeElement);

      Trail.makeTrailFromItem(items);

      expect(Tree.getTreeElementById).toHaveBeenCalledWith(467);
      expect(trail.hasClass('grb-trail')).toBeTruthy();
      expect($('#trail ol.breadcrumb').length).toBe(1);
    });

    it('includes add and edit if item is virtual', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      const treeElement = $('#first-child-one-one');
      items = {
        'parent_id': 467,
        'virtual': true
      };
      spyOn(Tree, 'getTreeElementById').and.returnValue(treeElement);

      Trail.makeTrailFromItem(items);

      expect($('#trail .grb-trail-add').length).toBe(1);
      expect($('#trail .grb-trail-edit').length).toBe(1);
    });

    it('includes edit autoscan if the item is db and autoscan', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      const treeElement = $('#first-child-one-one');
      items = {
        'parent_id': 467,
        'virtual': true,
        'autoscan_type': 'ui',
        'protect_container': false
      };
      spyOn(Tree, 'getTreeElementById').and.returnValue(treeElement);

      Trail.makeTrailFromItem(items);

      expect($('#trail .grb-trail-edit-autoscan').length).toBe(1);
    });
  });
  describe('trail click operations', () => {
    let items;
    beforeEach(() => {
      items = {
        'parent_id': 467,
        'location': '/Video',
        'virtual': true,
        'start': 0,
        'total_matches': 0,
        'autoscan_mode': 'timed',
        'autoscan_type': 'ui',
        'protect_container': false,
        'protect_items': false,
        'item': []
      };
      fixture.setBase('test/client/fixtures');
      fixture.load('trail.html');
      const treeElement = $('#first-child-one-one');
      spyOn(Tree, 'getTreeElementById').and.returnValue(treeElement);
    });
    afterEach(() => {
      fixture.cleanup();
    });

    it('when click delete calls Gerbera Item delete', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(Tree, 'reloadParentTreeItem');
      spyOn(Items, 'deleteGerberaItem').and.callFake(() => {
        return Promise.resolve({success: true});
      });

      Trail.makeTrailFromItem(items);
      $('#trail .grb-trail-delete').click();

      expect(Items.deleteGerberaItem).toHaveBeenCalled();
    });

    it('when click delete ALL calls Gerbera Item delete with ALL parameter set', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(Tree, 'reloadParentTreeItem');
      spyOn(Items, 'deleteGerberaItem').and.callFake(() => {
        return Promise.resolve({success: true});
      });

      Trail.makeTrailFromItem(items);
      $('#trail .grb-trail-delete-all').click();

      expect(Items.deleteGerberaItem).toHaveBeenCalledWith({ id: 11111, text: 'first.child.one.one' }, true);
    });

    it('when click add for fs type calls Gerbera Item add', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('fs');
      spyOn(Items, 'addFileItem');
      items = {
        'parent_id': 467,
        'virtual': false
      };
      Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-add').click();

      expect(Items.addFileItem).toHaveBeenCalled();
    });

    it('when click add autoscan for fs type calls Gerbera Autoscan add', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('fs');
      spyOn(Autoscan, 'addAutoscan');
      items = {
        'parent_id': 467,
        'virtual': false
      };
      Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-add-autoscan').click();

      expect(Autoscan.addAutoscan).toHaveBeenCalled();
    });

    it('when click add for db type calls Gerbera Items to add virtual', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(Items, 'addVirtualItem');
      items = {
        'parent_id': 467,
        'virtual': true
      };
      Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-add').click();

      expect(Items.addVirtualItem).toHaveBeenCalled();
    });

    it('when click edit calls Gerbera Item edit', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(Items, 'editItem').and.callFake(() => {
        return $.Deferred().resolve({success: true}).promise();
      });

      Trail.makeTrailFromItem(items);
      $('#trail .grb-trail-edit').click();

      expect(Items.editItem).toHaveBeenCalled();
    });

    it('when click add autoscan for fs type calls Gerbera Autoscan add', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(Autoscan, 'addAutoscan');
      items = {
        'parent_id': 467,
        'location': '/Video',
        'virtual': true,
        'start': 0,
        'total_matches': 0,
        'autoscan_mode': 'timed',
        'autoscan_type': 'ui',
        'protect_container': false,
        'protect_items': false,
        'item': []
      };
      Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-edit-autoscan').click();

      expect(Autoscan.addAutoscan).toHaveBeenCalled();
    });
  });
});
