var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The add new object', function () {
  var loginPage, homePage

  this.slow(8000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=add.object.spec.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/gerbera.html')

    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
  })

  test.after(function () {
    driver.quit()
  })

  test.it('a gerbera tree db item container adds new object with parent id', function () {
    var itemCountBefore = 13
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.items().then(function (items) {
      expect(items.length).to.equal(itemCountBefore)
    })
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.editOverlayFieldValue('addParentId').then(function (value) {
          expect(value).to.equal('7443')
        })
        homePage.editOverlayTitle().then(function (title) {
          expect(title).to.equal('Add Item')
        })
        homePage.setEditorOverlayField('editTitle', 'A Container')
        homePage.submitEditor()
        homePage.items().then(function (items) {
          expect(items.length).to.equal(itemCountBefore + 1)
        })

        homePage.getToastMessage().then(function (message) {
          expect(message).to.equal('Successfully added object')
        })

        homePage.waitForToastClose().then(function (displayed) {
          expect(displayed).to.be.false
        })
      })
    })
  })
})
