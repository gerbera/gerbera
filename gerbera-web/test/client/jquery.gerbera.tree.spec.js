/* global jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('The jQuery Tree', function () {
  'use strict'
  var treeData
  var treeConfig = {
    titleClass: 'folder-title',
    closedIcon: 'folder-closed',
    openIcon: 'folder-open'
  }

  beforeEach(function () {
    loadFixtures('tree.html')
    loadJSONFixtures('tree-data.json')
    treeData = getJSONFixture('tree-data.json')
  })

  it('creates a list the same size as the data', function () {

    $('#tree').tree({
      data: treeData,
      config: treeConfig
    })

    expect($('#tree').find('li').length).toBe(5)
  })

  it('items are loaded in folder closed state', function () {
    $('#tree').tree({
      data: treeData,
      config: treeConfig
    })

    var items = $('#tree').find('li')
    var folder = items.children('span')

    expect(folder.hasClass('folder-closed')).toBeTruthy()
  })

  it('binds the item title text to a onSelection method', function () {
    var onSelectSpy = jasmine.createSpy('onSelection')

    $('#tree').tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed',
        onSelection: onSelectSpy
      }
    })

    var titles = $('#tree').find('.folder-title')
    $(titles.get(0)).click()

    expect(onSelectSpy.calls.count()).toBe(1)
  })

  it('binds the folder title to a onExpand method', function () {
    var onExpand = jasmine.createSpy('onExpand')

    $('#tree').tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed',
        onExpand: onExpand
      }
    })

    var icons = $('#tree').find('.folder-title')
    $(icons.get(0)).click()

    expect(onExpand.calls.count()).toBe(1)
  })

  it('appends badges to the item', function () {
    $('#tree').tree({
      data: treeData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed'
      }
    })

    var items = $('#tree').find('li')
    var firstBadges = $(items.get(0)).find('span.badge')
    expect(firstBadges.length).toBe(2)
  })

  it('recursively appends children based on data lineage', function () {
    loadJSONFixtures('tree-data-hierarchy.json')
    var treeHierarchyData = getJSONFixture('tree-data-hierarchy.json')

    $('#tree').tree({
      data: treeHierarchyData,
      config: {
        titleClass: 'folder-title',
        closedIcon: 'folder-closed'
      }
    })
    var firstChild = $('#tree').children('ul').children('li')

    expect(firstChild.length).toBe(1)

    var secondChild = firstChild.children('ul')
    var secondChildChild = secondChild.children('li')
    expect(secondChild.length).toBe(1)
    expect(secondChildChild.length).toBe(1)
  })

  describe('appendItems()', function () {
    var treeData

    beforeEach(function () {
      loadFixtures('tree.html')
      loadJSONFixtures('tree-data.json')
      treeData = getJSONFixture('tree-data.json')
    })

    it('given a parent item append children as list', function () {
      $('#tree').tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed',
          openIcon: 'folder-open'
        }
      })

      var parent = $($('#tree').find('li').get(4))
      var children = [
        {'title': 'Folder 6'},
        {'title': 'Folder 7'},
        {'title': 'Folder 8'},
        {'title': 'Folder 9'}
      ]

      $('#tree').tree('append', parent, children)

      var newList = parent.children('ul')
      expect(newList.length).toBe(1)
      expect(newList.children('li').length).toBe(4)
      expect($('#tree').tree('closed', parent.children('span').first())).toBeFalsy()
    })
  })

  describe('clear()', function () {
    var treeData

    beforeEach(function () {
      loadFixtures('tree.html')
      loadJSONFixtures('tree-data.json')
      treeData = getJSONFixture('tree-data.json')
    })

    it('removes all contents of the tree', function () {
      $('#tree').tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      })

      expect($('#tree').find('li').length).toBe(5)

      $('#tree').tree('clear')

      expect($('#tree').find('li').length).toBe(0)
    })
  })

  describe('closed()', function () {
    var treeData

    beforeEach(function () {
      loadFixtures('tree.html')
      loadJSONFixtures('tree-data.json')
      treeData = getJSONFixture('tree-data.json')
    })

    it('determines whether a node is closed', function () {
      $('#tree').tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed'
        }
      })

      var item = $('#tree').find('li').get(0)
      var title = $(item).children('span').get(1)
      expect($('#tree').tree('closed', title)).toBeTruthy()
    })
  })

  describe('collapse()', function () {
    var treeData

    beforeEach(function () {
      loadFixtures('tree.html')
      loadJSONFixtures('tree-data.json')
      treeData = getJSONFixture('tree-data.json')
    })

    it('given a parent item append children as list', function () {
      $('#tree').tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed',
          openIcon: 'folder-open'
        }
      })

      var parent = $($('#tree').find('li').get(4))
      var children = [
        {'title': 'Folder 6'},
        {'title': 'Folder 7'},
        {'title': 'Folder 8'},
        {'title': 'Folder 9'}
      ]
      $('#tree').tree('append', parent, children)

      var title = parent.children('span').get(1)
      $('#tree').tree('collapse', parent)

      expect($('#tree').tree('closed', title)).toBeTruthy()
      expect(parent.children('ul.list-group').css('display')).toBe('none')
    })
  })

  describe('select()', function () {
    var treeData

    beforeEach(function () {
      loadFixtures('tree.html')
      loadJSONFixtures('tree-data.json')
      treeData = getJSONFixture('tree-data.json')
    })

    it('selects an item by changing the style', function () {
      $('#tree').tree({
        data: treeData,
        config: {
          titleClass: 'folder-title',
          closedIcon: 'folder-closed',
          openIcon: 'folder-open'
        }
      })

      var item = $($('#tree').find('li').get(4))

      $('#tree').tree('select', item.children('span.folder-title'))

      expect(item.hasClass('selected-item')).toBeTruthy()
    })
  })
})
