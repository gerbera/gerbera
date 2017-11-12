/* global jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('The jQuery Datagrid', function () {
  'use strict'
  var datagridData

  beforeEach(function () {
    loadFixtures('datagrid.html')
    loadJSONFixtures('datagrid-data.json')
    datagridData = getJSONFixture('datagrid-data.json')
  })

  it('creates a table based on the dataset provided', function () {
    $('#datagrid').dataitems({
      data: datagridData
    })

    expect($('#datagrid').find('tr').length).toBe(12)
    expect($('#datagrid').find('tr.grb-item').get(0).innerText).toContain(datagridData[0].text) // skip the header row
    expect($('#datagrid').find('a').get(0).href).toBe(datagridData[0].url)
  })

  it('binds the delete icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('delete')
    $('#datagrid').dataitems({
      data: datagridData,
      onDelete: methodSpy,
      itemType: 'db'
    })

    $('#datagrid').find('.grb-item span.grb-item-delete').first().click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('does not bind delete on file system types', function () {
    var methodSpy = jasmine.createSpy('delete')
    $('#datagrid').dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'fs'
    })

    $('#datagrid').find('.grb-item span.grb-item-delete').first().click()

    expect(methodSpy).not.toHaveBeenCalled()
  })

  it('binds the edit icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('edit')
    $('#datagrid').dataitems({
      data: datagridData,
      onEdit: methodSpy,
      itemType: 'db'
    })

    $('#datagrid').find('.grb-item span.grb-item-edit').first().click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('does not bind edit on file system types', function () {
    var methodSpy = jasmine.createSpy('edit')
    $('#datagrid').dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'fs'
    })

    $('#datagrid').find('.grb-item span.grb-item-edit').first().click()

    expect(methodSpy).not.toHaveBeenCalled()
  })

  it('binds the download icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('download')
    $('#datagrid').dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'db'
    })

    $('#datagrid').find('.grb-item span.grb-item-download').first().click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('does not bind download on file system types', function () {
    var methodSpy = jasmine.createSpy('download')
    $('#datagrid').dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'fs'
    })

    $('#datagrid').find('.grb-item span.grb-item-download').first().click()

    expect(methodSpy).not.toHaveBeenCalled()
  })

  it('binds the add icon click to the method provided', function () {
    var methodSpy = jasmine.createSpy('add')
    $('#datagrid').dataitems({
      data: datagridData,
      onAdd: methodSpy,
      itemType: 'fs'
    })

    $('#datagrid').find('.grb-item span.grb-item-add').first().click()

    expect(methodSpy).toHaveBeenCalled()
  })

  it('does not bind add icon to db types', function () {
    var methodSpy = jasmine.createSpy('add')
    $('#datagrid').dataitems({
      data: datagridData,
      onAdd: methodSpy,
      itemType: 'db'
    })

    $('#datagrid').find('.grb-item span.grb-item-add').first().click()

    expect(methodSpy).not.toHaveBeenCalled()
  })

  it('creates a table with a footer container a pager', function () {
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 1,
        totalMatches: 1000,
        itemsPerPage: 10}
    })

    expect($('#datagrid').find('tfoot').length).toBe(1)
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(102)
  })

  it('creates a pager with only page count pages', function () {
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 1,
        totalMatches: 1000,
        itemsPerPage: 10}
    })

    expect($('#datagrid').find('tfoot').length).toBe(1)
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(102)
  })

  it('creates a pager with only necessary amount of pages', function () {
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 1,
        totalMatches: 20,
        itemsPerPage: 10}
    })

    expect($('#datagrid').find('tfoot').length).toBe(1)
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(4)
  })

  it('creates a pager with existing number of pages', function () {
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 2,
        totalMatches: 20,
        itemsPerPage: 10}
    })

    expect($('#datagrid').find('tfoot').length).toBe(1)
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(4)
  })

  it('does not create a pager when no page info exists', function () {
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {pageCount: 0}
    })

    expect($('#datagrid nav.grb-pager').length).toBe(0)
  })

  it('does not create a pager when no page info exists', function () {
    $('#datagrid').dataitems({
      data: datagridData
    })

    expect($('#datagrid nav.grb-pager').length).toBe(0)
  })

  it('creates a pager that goes to specific page when clicked', function () {
    var clickSpy = jasmine.createSpy('click')
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {
        currentPage: 1,
        pageCount: 10,
        onClick: clickSpy,
        totalMatches: 1000,
        itemsPerPage: 10
      }
    })

    $($('#datagrid nav.grb-pager > ul > li').get(1)).find('a').click()

    expect(clickSpy).toHaveBeenCalled()
  })

  it('creates a pager that goes to previous page when clicked', function () {
    var clickSpy = jasmine.createSpy('click')
    var nextSpy = jasmine.createSpy('next')
    var previousSpy = jasmine.createSpy('previous')
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {
        currentPage: 1,
        pageCount: 10,
        onClick: clickSpy,
        onNext: nextSpy,
        onPrevious: previousSpy,
        totalMatches: 1000,
        itemsPerPage: 10
      }
    })

    // the << previous directional
    $($('#datagrid nav.grb-pager > ul > li').get(0)).find('a').click()

    expect(previousSpy).toHaveBeenCalled()
  })

  it('creates a pager that goes to next page when clicked', function () {
    var clickSpy = jasmine.createSpy('click')
    var nextSpy = jasmine.createSpy('next')
    var previousSpy = jasmine.createSpy('previous')
    $('#datagrid').dataitems({
      data: datagridData,
      pager: {
        currentPage: 1,
        pageCount: 10,
        onClick: clickSpy,
        onNext: nextSpy,
        onPrevious: previousSpy,
        totalMatches: 20,
        itemsPerPage: 10
      }
    })

    // the >> next directional
    $($('#datagrid nav.grb-pager > ul > li').get(3)).find('a').click()

    expect(nextSpy).toHaveBeenCalled()
  })
})
