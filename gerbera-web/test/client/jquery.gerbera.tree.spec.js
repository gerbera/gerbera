import treeDataJson from './fixtures/tree-data';
import treeDataHierarchy from './fixtures/tree-data-hierarchy';
import treeDataDiffJson from './fixtures/tree-data-diff';

describe('The jQuery Tree', () => {
  'use strict';

  let treeData, tree;
  const treeConfig = {
    titleClass: 'folder-title',
    closedIcon: 'folder-closed',
    openIcon: 'folder-open'
  };

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    treeData = treeDataJson;
    tree = $('#tree');
  });

  afterEach(() => {
    fixture.cleanup();
  });

  it('creates a list the same size as the data', () => {

    tree.tree({
      data: treeData,
      config: treeConfig
    });

    expect(tree.find('li').length).toBe(5);
  });

  it('items are loaded in folder closed state', () => {
    tree.tree({
      data: treeData,
      config: treeConfig
    });

    const items = $('#tree').find('li');
    const folder = items.children('span');

    expect(folder.hasClass('folder-closed')).toBeTruthy();
  });

  it('binds the item title text to a onSelection method', () => {
    const onSelectSpy = jasmine.createSpy('onSelection');

    tree.tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed',
        onSelection: onSelectSpy
      }
    });

    const titles = tree.find('.folder-title');
    $(titles.get(0)).click();

    expect(onSelectSpy.calls.count()).toBe(1);
  });

  it('binds the folder title to a onExpand method', () => {
    const onExpand = jasmine.createSpy('onExpand');

    tree.tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed',
        onExpand: onExpand
      }
    });

    const icons = tree.find('.folder-title');
    $(icons.get(0)).click();

    expect(onExpand.calls.count()).toBe(1);
  });

  it('appends badges to the item', () => {
    tree.tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed'
      }
    });

    const items = $('#tree').find('li');
    const firstBadges = $(items.get(0)).find('span.badge');
    expect(firstBadges.length).toBe(3);
  });

  it('binds autoscan edit to the autoscan badge', () => {
    const onAutoscanEdit = jasmine.createSpy('onAutoscanEdit');

    tree.tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed',
        onAutoscanEdit: onAutoscanEdit
      }
    });

    const autoscanBadge = tree.find('.autoscan');
    $(autoscanBadge.get(0)).click();

    expect(onAutoscanEdit.calls.count()).toBe(1);
  });

  it('appends the gerbera ID as a data item', () => {
    tree.tree({
      data: treeData,
      config: {}
    });

    const items = $('#tree').find('li');
    const gerberaId = $(items.get(0)).data('grb-id');
    expect(gerberaId).toEqual(1111);
  });

  it('recursively appends children based on data lineage', () => {
    const treeHierarchyData = treeDataHierarchy;

    tree.tree({
      data: treeHierarchyData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed'
      }
    });
    const firstChild = $('#tree').children('ul').children('li');

    expect(firstChild.length).toBe(1);

    const secondChild = firstChild.children('ul');
    const secondChildChild = secondChild.children('li');
    expect(secondChild.length).toBe(1);
    expect(secondChildChild.length).toBe(1);
  });

  describe('appendItems()', () => {

    it('given a parent item append children as list', () => {
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed',
          openIcon: 'folder-open'
        }
      });

      const parent = $(tree.find('li').get(4));
      const children = [
        {'title': 'Folder 6', 'gerbera': {id: 6}},
        {'title': 'Folder 7', 'gerbera': {id: 7}},
        {'title': 'Folder 8', 'gerbera': {id: 8}},
        {'title': 'Folder 9', 'gerbera': {id: 9}}
      ];

      tree.tree('append', parent, children);

      const newList = parent.children('ul');
      expect(newList.length).toBe(1);
      expect(newList.children('li').length).toBe(4);
      expect(tree.tree('closed', parent.children('span').first())).toBeFalsy();
    });
  });

  describe('clear()', () => {

    it('removes all contents of the tree', () => {
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });

      expect(tree.find('li').length).toBe(5);

      tree.tree('clear');

      expect(tree.find('li').length).toBe(0);
    });
  });

  describe('closed()', () => {

    it('determines whether a node is closed', () => {
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });

      const item = tree.find('li').get(0);
      const title = $(item).children('span').get(1);
      expect(tree.tree('closed', title)).toBeTruthy();
    });
  });

  describe('collapse()', () => {

    it('given a parent item append children as list', () => {
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed',
          openIcon: 'folder-open'
        }
      });

      const parent = $($('#tree').find('li').get(4));
      const children = [
        {'title': 'Folder 6', 'gerbera': {id: 6}},
        {'title': 'Folder 7', 'gerbera': {id: 7}},
        {'title': 'Folder 8', 'gerbera': {id: 8}},
        {'title': 'Folder 9', 'gerbera': {id: 9}}
      ];
      tree.tree('append', parent, children);

      const title = parent.children('span').get(1);
      tree.tree('collapse', parent);

      expect(tree.tree('closed', title)).toBeTruthy();
      expect(parent.children('ul.list-group').length).toBe(0);
    });
  });

  describe('select()', () => {

    it('selects an item by changing the style', () => {
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed',
          openIcon: 'folder-open'
        }
      });

      const item = $($('#tree').find('li').get(4));

      tree.tree('select', item);

      expect(item.hasClass('selected-item')).toBeTruthy();
    });
  });

  describe('getItem()', () => {
    let treeDataDiff;

    beforeEach(() => {
      treeDataDiff = treeDataDiffJson;
      tree = $('#tree');
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });
    });

    it('stores the item data and finds given id', () => {
      const folderData = {
        title: 'Folder 1',
        badge: [
          'test',
          'test1',
          'a'
        ],
        gerbera: {
          id: 1111,
          childCount: 0,
          autoScanMode: 'none',
          autoScanType: 'ui'
        }
      };

      const folderItem = tree.tree('getItem', 1111);
      expect(folderItem).toEqual(folderData);
    });

    it('clears data after regenerating the tree and loads the new data', () => {
      const folderData = {
        title: 'Folder 1',
        badge: [
          'test',
          'test1',
          'a'
        ],
        gerbera: {
          id: 1111,
          childCount: 0,
          autoScanMode: 'none',
          autoScanType: 'ui'
        }
      };

      let treeItem = tree.tree('getItem', 1111);
      expect(treeItem).toEqual(folderData);

      tree.tree('destroy');

      tree.tree({
        data: treeDataDiff,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });

      treeItem = tree.tree('getItem', 1111);
      expect(treeItem).toBeFalsy();

      treeItem = tree.tree('getItem', 9999);
      expect(treeItem).toBeDefined();
    });
  });

  describe('getElement()', () => {

    beforeEach(() => {
      tree = $('#tree');
      tree.tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      });
    });

    it('finds the HTML element given gerbera data item', () => {
      const treeElement = tree.tree('getElement', 1111);

      expect(treeElement.length).toBe(1);
      expect(treeElement.prop('id')).toBe('grb-tree-1111');
    });
  });
});
