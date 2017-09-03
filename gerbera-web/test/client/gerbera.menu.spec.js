/* global GERBERA spyOn jasmine it expect describe beforeEach loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera Menu', function () {
  'use strict'
  describe('initialize()', function () {

    beforeEach(function () {
      loadFixtures('index.html')
    })

    it('binds all menu items with the click event', function (done) {
      spyOn(GERBERA.Menu, 'click')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.Menu.initialize().then(function () {
        $('#nav-db').click()
        expect(GERBERA.Menu.click).toHaveBeenCalledWith(jasmine.any(Object))
        done()
      })
    })

    it('on click of Database calls to load the containers', function (done) {
      spyOn(GERBERA.Tree, 'selectType')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.Menu.initialize().then(function () {
        $('#nav-db').click()
        expect(GERBERA.Tree.selectType).toHaveBeenCalledWith('db', 0)
        done()
      })
    })

    it('on click of menu, clears items', function (done) {
      spyOn(GERBERA.Items, 'destroy')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.Menu.initialize().then(function () {
        $('#nav-db').click()
        expect(GERBERA.Items.destroy).toHaveBeenCalled()
        done()
      })
    })

    it('on click of home menu, clears tree and items', function (done) {
      spyOn(GERBERA.Items, 'destroy')
      spyOn(GERBERA.Tree, 'destroy')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.Menu.initialize().then(function () {
        $('#nav-home').click()
        expect(GERBERA.Items.destroy).toHaveBeenCalled()
        expect(GERBERA.Tree.destroy).toHaveBeenCalled()
        done()
      })
    })

  })
})
