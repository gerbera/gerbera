/* global jasmine it expect describe beforeEach afterEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('The jQuery Trail', function () {
  'use strict'
  var trailData

  beforeEach(function () {
    loadFixtures('trail.html')
    loadJSONFixtures('trail-data.json')
    trailData = getJSONFixture('trail-data.json')
  })

  afterEach(function () {
    if ($('#trail').hasClass('grb-trail')) {
      $('#trail').trail('destroy')
    }
  })

  it('Uses an array of items to build the breadcrumb', function () {
    var trailConfig = {}

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    expect($('#trail').trail('length')).toBe(4)
  })

  it('should display in the proper order', function () {
    var trailConfig = {}
    trailData = [
      { text: 'First Item',
        gerbera: {
          id: 0
        }
      },
      { text: 'Second Item',
        gerbera: {
          id: 0
        }
      }
    ]

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    var items = $('#trail').trail('items')
    expect(items.first().text()).toBe('First Item')
    expect(items.last().text()).toBe('Second Item')
  })

  it('generates add icons when configured', function () {
    var trailConfig = {
      enableAdd: true,
      onAdd: function () { return false },
      enableAddAutoscan: true,
      onAddAutoscan: function () { return false }
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    expect($('#trail .grb-trail-add').length).toBe(1)
    expect($('#trail .grb-trail-add-autoscan').length).toBe(1)
  })

  it('does not generate add icons when not enabled by configuration', function () {
    var trailConfig = {
      enableAdd: false,
      enableAddAutoscan: false
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    expect($('#trail .grb-trail-add').length).toBe(0)
    expect($('#trail .grb-trail-add-autoscan').length).toBe(0)
  })

  it('generates delete icon when enabled by configuration', function () {
    var trailConfig = {
      enableDelete: true,
      onDelete: function () { return false },
      enableDeleteAll: true,
      onDeleteAll: function () { return false },
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    expect($('#trail .grb-trail-delete').length).toBe(1)
    expect($('#trail .grb-trail-delete-all').length).toBe(1)
  })

  it('binds the delete icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('delete')
    var trailConfig = {
      enableDelete: true,
      onDelete: methodSpy
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    $('#trail').find('a.grb-trail-delete').click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('binds the delete all icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('deleteAll')
    var trailConfig = {
      enableDeleteAll: true,
      onDeleteAll: methodSpy
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    $('#trail').find('a.grb-trail-delete-all').click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('does not generate delete icons when not configured', function () {
    var trailConfig = {
      enableDelete: false,
      enableDeleteAll: false
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    expect($('#trail .grb-trail-delete').length).toBe(0)
    expect($('#trail .grb-trail-delete-all').length).toBe(0)
  })

  it('binds the add icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('add')
    var trailConfig = {
      enableAdd: true,
      onAdd: methodSpy
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    $('#trail').find('a.grb-trail-add').click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('binds the add autoscan icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('addAutoscan')
    var trailConfig = {
      enableAddAutoscan: true,
      onAddAutoscan: methodSpy
    }

    $('#trail').trail({
      data: trailData,
      config: trailConfig
    })

    $('#trail').find('a.grb-trail-add-autoscan').click()

    expect(methodSpy).toHaveBeenCalled()
  })

  describe('length()', function () {
    it('reports the length of the trail of items', function () {
      var trailConfig = {}

      $('#trail').trail({
        data: trailData,
        config: trailConfig
      })
      expect($('#trail').trail('length')).toBe(4)
    })
  })

  describe('items()', function () {
    it('returns the list of items', function () {
      var trailConfig = {}

      $('#trail').trail({
        data: trailData,
        config: trailConfig
      })

      var items = $('#trail').trail('items')
      expect(items.length).toBe(4)
      expect(items.first().text()).toBe('database')
    })
  })

  describe('destroy()', function () {
    it('removes all contents of the parent', function () {
      var trailConfig = {}

      $('#trail').trail({
        data: trailData,
        config: trailConfig
      })
      expect($('#trail').trail('length')).toBe(4)

      $('#trail').trail('destroy')

      expect($('#trail').children().length).toBe(0)
      expect($('#trail').hasClass('grb-trail')).toBeFalsy()
    })
  })
})
