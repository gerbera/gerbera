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

    expect($('#datagrid').find('tr').length).toBe(11)
    expect($('#datagrid').find('tr').get(1).innerText).toBe(datagridData[0].text) // skip the header row
    expect($('#datagrid').find('a').get(0).href).toBe(datagridData[0].url)
  })
})
