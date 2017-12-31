/* global GERBERA spyOn jasmine it expect describe beforeEach loadFixtures loadJSONFixtures getJSONFixture */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('Gerbera Menu', function () {
  'use strict'
  describe('initialize()', function () {

    beforeEach(function () {
      loadFixtures('index.html')
      spyOn(GERBERA.Updates, 'getUpdates')
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

      loadJSONFixtures('parent_id-0-select_it-0.json')
      var response = getJSONFixture('parent_id-0-select_it-0.json')
      spyOn($, 'ajax').and.callFake(function (options) {
        return $.Deferred().resolve(response).promise()
      })

      GERBERA.Menu.initialize().then(function () {
        $('#nav-db').click()
        expect(GERBERA.Items.destroy).toHaveBeenCalled()
        done()
      })
    })

    it('on click of home menu, clears tree and items', function (done) {
      spyOn(GERBERA.Items, 'destroy')
      spyOn(GERBERA.Tree, 'destroy')
      spyOn(GERBERA.Trail, 'destroy')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.Menu.initialize().then(function () {
        $('#nav-home').click()
        expect(GERBERA.Items.destroy).toHaveBeenCalled()
        expect(GERBERA.Tree.destroy).toHaveBeenCalled()
        expect(GERBERA.Trail.destroy).toHaveBeenCalled()
        done()
      })
    })

    it('on click of leave beta destroys elements, and navigates to index.html page', function (done) {
      spyOn(GERBERA.Items, 'destroy')
      spyOn(GERBERA.Tree, 'destroy')
      spyOn(GERBERA.Trail, 'destroy')
      spyOn(GERBERA.App, 'reload')
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true)

      GERBERA.Menu.initialize().then(function () {
        $('#leave-beta').click()
        expect(GERBERA.Tree.destroy).toHaveBeenCalled()
        expect(GERBERA.Trail.destroy).toHaveBeenCalled()
        expect(GERBERA.Items.destroy).toHaveBeenCalled()
        expect(GERBERA.App.reload).toHaveBeenCalledWith('/index.html')
        done()
      })
    })
  })

  describe('disable()', function () {

    beforeEach(function () {
      loadFixtures('index.html')
      spyOn(GERBERA.Updates, 'getUpdates')
    })

    it('disables all menu items except the report issue link', function () {
      GERBERA.Menu.disable()

      var menuItems = ['nav-home', 'nav-db', 'nav-fs']

      $.each(menuItems, function (index, value) {
        var link = $('#'+ value)
        expect(link.hasClass('disabled')).toBeTruthy()
      })
      expect( $('#report-issue').hasClass('disabled')).toBeFalsy()
    })
  })

  describe('hideLogin()', function () {

    beforeEach(function () {
      loadFixtures('index.html')
      spyOn(GERBERA.Updates, 'getUpdates')
    })

    it('hides login fields when called', function () {
      GERBERA.Menu.hideLogin()

      expect($('.login-field').is(':visible')).toBeFalsy()
      expect($('#login-submit').is(':visible')).toBeFalsy()
      expect($('#logout').is(':visible')).toBeFalsy()
    })
  })
})
