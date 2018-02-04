var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The gerbera trail', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(60000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=trail.spec.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/index.html')

    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
  })

  test.after(function () {
    driver.quit()
  })

  test.it('a gerbera tree item shows an add icon in the trail but not delete item', function () {
    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc')
    homePage.hasTrailAddIcon().then(function (hasAdd) {
      expect(hasAdd).to.be.true
    })
    homePage.hasTrailAddAutoscanIcon().then(function (hasAdd) {
      expect(hasAdd).to.be.true
    })
    homePage.hasTrailDeleteIcon().then(function (hasDelete) {
      expect(hasDelete).to.be.false
    })
  })

  test.it('a gerbera tree db item container shows delete icon and add icon in the trail', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.hasTrailDeleteIcon().then(function (hasDelete) {
      expect(hasDelete).to.be.true
    })
    homePage.hasTrailAddIcon().then(function (has) {
      expect(has).to.be.true
    })
  })
})
