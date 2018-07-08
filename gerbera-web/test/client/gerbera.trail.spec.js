/* global GERBERA jasmine it expect describe beforeEach loadJSONFixtures loadFixtures getJSONFixture spyOn */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('Gerbera Trail', () => {
  'use strict';

  describe('initialize()', () => {
    beforeEach(() => {
      loadFixtures('trail.html');
    });
    it('clears any data associated with the trail', () => {
      const trail = $('#trail');

      GERBERA.Trail.initialize();

      expect(trail.children().length).toBe(0)
    });
  });

  describe('makeTrail()', () => {
    beforeEach(() => {
      loadFixtures('trail.html');
    });
    it('creates a trail of elements given a selected item in the tree', () => {
      const treeElement = $('#first-child-one-one');
      const trail = $('#trail');

      GERBERA.Trail.makeTrail(treeElement);

      expect(trail.hasClass('grb-trail')).toBeTruthy();
      expect($('#trail ol.breadcrumb').length).toBe(1);
    });
  });

  describe('destroy()', () => {
    let treeElement, trail;

    beforeEach(() => {
      treeElement = $('#first-child-one-one');
      trail = $('#trail');
      loadFixtures('trail.html');
      GERBERA.Trail.makeTrail(treeElement);
    });

    it('removes contents of the trail', () => {
      GERBERA.Trail.destroy();

      expect(trail.children().length).toBe(0);
    });
  });

  describe('gatherTrail()', () => {
    it('collects trail data from the tree', () => {
      loadFixtures('trail.html');
      const treeElement = $('#first-child-one-one');
      const expected = [
        { id: 111, text: 'first' },
        { id: 1111, text: 'first.child.one' },
        { id: 11111, text: 'first.child.one.one' }
      ];

      const result = GERBERA.Trail.gatherTrail(treeElement);

      expect(result).toEqual(expected);
    });

    it('collects trail data from the tree that has non-numeric IDs', () => {
      loadFixtures('trail-fs.html');
      const treeElement = $('#first-child-one-one');
      const expected = [
        { id: 'aaa', text: 'first' },
        { id: 'aaaa', text: 'first.child.one' },
        { id: 'aaaaa', text: 'first.child.one.one' }
      ];

      const result = GERBERA.Trail.gatherTrail(treeElement);

      expect(result).toEqual(expected);
    });
  });

  describe('deleteItem()', () => {
    let deleteResponse, event, treeResponse;
    beforeEach(() => {
      deleteResponse = { success: true };
      event = {data: { id: 8 }};
      loadJSONFixtures('parent_id-0-select_it-0.json');
      treeResponse = getJSONFixture('parent_id-0-select_it-0.json');
      loadFixtures('trail.html');
    });
    it('calls the server to delete the gerbera item', async () => {
      GERBERA.Tree.loadTree(treeResponse);
      spyOn(GERBERA.Items, 'deleteGerberaItem').and.callFake(() => {
        return $.Deferred().resolve(deleteResponse).promise();
      });
      spyOn(GERBERA.App, 'error');
      spyOn(GERBERA.Updates, 'updateTreeByIds');
      spyOn(GERBERA.Updates, 'showMessage');

      await GERBERA.Trail.deleteItem(event);

      expect(GERBERA.App.error).not.toHaveBeenCalled();
      expect(GERBERA.Items.deleteGerberaItem).toHaveBeenCalledWith(event.data);
      expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled();
      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Successfully removed item', undefined, 'success', 'fa-check');
    });

    describe('deleteAllItems()', () => {
      let deleteResponse, event, treeResponse;
      beforeEach(() => {
        deleteResponse = {success: true};
        event = {data: {id: 8}};
        loadJSONFixtures('parent_id-0-select_it-0.json');
        treeResponse = getJSONFixture('parent_id-0-select_it-0.json');
        loadFixtures('trail.html');
      });
      it('calls the server to delete the gerbera item', async () => {
        GERBERA.Tree.loadTree(treeResponse);
        spyOn(GERBERA.Items, 'deleteGerberaItem').and.callFake(() => {
          return $.Deferred().resolve(deleteResponse).promise();
        });
        spyOn(GERBERA.App, 'error');
        spyOn(GERBERA.Updates, 'updateTreeByIds');
        spyOn(GERBERA.Updates, 'showMessage');

        await GERBERA.Trail.deleteAllItems(event);
        expect(GERBERA.App.error).not.toHaveBeenCalled();
        expect(GERBERA.Items.deleteGerberaItem).toHaveBeenCalledWith(event.data, true);
        expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled();
        expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Successfully removed all items', undefined, 'success', 'fa-check');
      });
    });

    it('reports an error if the delete fails', () => {
      GERBERA.Tree.loadTree(treeResponse);
      deleteResponse = { success: false };
      spyOn(GERBERA.Items, 'deleteGerberaItem').and.callFake(() => {
        return $.Deferred().resolve(deleteResponse).promise();
      });
      spyOn(GERBERA.App, 'error');
      spyOn(GERBERA.Updates, 'updateTreeByIds');

      GERBERA.Trail.deleteItem(event);

      expect(GERBERA.App.error).toHaveBeenCalledWith('Failed to remove item');
      expect(GERBERA.Updates.updateTreeByIds).not.toHaveBeenCalled();
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
      loadFixtures('trail.html');
      trail = $('#trail');
    });

    it('uses and item response to make the trail', () => {
      const treeElement = $('#first-child-one-one');
      spyOn(GERBERA.Tree, 'getTreeElementById').and.returnValue(treeElement);

      GERBERA.Trail.makeTrailFromItem(items);

      expect(GERBERA.Tree.getTreeElementById).toHaveBeenCalledWith(467);
      expect(trail.hasClass('grb-trail')).toBeTruthy();
      expect($('#trail ol.breadcrumb').length).toBe(1);
    });

    it('includes add and edit if item is virtual', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      const treeElement = $('#first-child-one-one');
      items = {
        'parent_id': 467,
        'virtual': true
      };
      spyOn(GERBERA.Tree, 'getTreeElementById').and.returnValue(treeElement);

      GERBERA.Trail.makeTrailFromItem(items);

      expect($('#trail .grb-trail-add').length).toBe(1);
      expect($('#trail .grb-trail-edit').length).toBe(1);
    });

    it('includes edit autoscan if the item is db and autoscan', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      const treeElement = $('#first-child-one-one');
      items = {
        'parent_id': 467,
        'virtual': true,
        'autoscan_type': 'ui',
        'protect_container': false
      };
      spyOn(GERBERA.Tree, 'getTreeElementById').and.returnValue(treeElement);

      GERBERA.Trail.makeTrailFromItem(items);

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
      loadFixtures('trail.html');
      const treeElement = $('#first-child-one-one');
      spyOn(GERBERA.Tree, 'getTreeElementById').and.returnValue(treeElement);
    });

    it('when click delete calls Gerbera Item delete', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      spyOn(GERBERA.Tree, 'reloadParentTreeItem');
      spyOn(GERBERA.Items, 'deleteGerberaItem').and.callFake(() => {
        return $.Deferred().resolve({success: true}).promise();
      });

      GERBERA.Trail.makeTrailFromItem(items);
      $('#trail .grb-trail-delete').click();

      expect(GERBERA.Items.deleteGerberaItem).toHaveBeenCalled();
    });

    it('when click delete ALL calls Gerbera Item delete with ALL parameter set', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      spyOn(GERBERA.Tree, 'reloadParentTreeItem');
      spyOn(GERBERA.Items, 'deleteGerberaItem').and.callFake(() => {
        return $.Deferred().resolve({success: true}).promise();
      });

      GERBERA.Trail.makeTrailFromItem(items);
      $('#trail .grb-trail-delete-all').click();

      expect(GERBERA.Items.deleteGerberaItem).toHaveBeenCalledWith({ id: 11111, text: 'first.child.one.one' }, true);
    });

    it('when click add for fs type calls Gerbera Item add', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('fs');
      spyOn(GERBERA.Items, 'addFileItem');
      items = {
        'parent_id': 467,
        'virtual': false
      };
      GERBERA.Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-add').click();

      expect(GERBERA.Items.addFileItem).toHaveBeenCalled();
    });

    it('when click add autoscan for fs type calls Gerbera Autoscan add', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('fs');
      spyOn(GERBERA.Autoscan, 'addAutoscan');
      items = {
        'parent_id': 467,
        'virtual': false
      };
      GERBERA.Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-add-autoscan').click();

      expect(GERBERA.Autoscan.addAutoscan).toHaveBeenCalled();
    });

    it('when click add for db type calls Gerbera Items to add virtual', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      spyOn(GERBERA.Items, 'addVirtualItem');
      items = {
        'parent_id': 467,
        'virtual': true
      };
      GERBERA.Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-add').click();

      expect(GERBERA.Items.addVirtualItem).toHaveBeenCalled();
    });

    it('when click edit calls Gerbera Item edit', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      spyOn(GERBERA.Items, 'editItem').and.callFake(() => {
        return $.Deferred().resolve({success: true}).promise();
      });

      GERBERA.Trail.makeTrailFromItem(items);
      $('#trail .grb-trail-edit').click();

      expect(GERBERA.Items.editItem).toHaveBeenCalled();
    });

    it('when click add autoscan for fs type calls Gerbera Autoscan add', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      spyOn(GERBERA.Autoscan, 'addAutoscan');
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
      GERBERA.Trail.makeTrailFromItem(items);

      $('#trail .grb-trail-edit-autoscan').click();

      expect(GERBERA.Autoscan.addAutoscan).toHaveBeenCalled();
    });
  });
});
