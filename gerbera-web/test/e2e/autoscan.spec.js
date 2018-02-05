var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The autoscan overlay', function () {
  var loginPage, homePage

  this.slow(7000)
  this.timeout(60000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=autoscan.spec.json')

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

  test.it('a gerbera tree fs item container opens autoscan overlay when trail add autoscan is clicked', function () {
    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc')
    homePage.clickTrailAddAutoscan().then(function () {
      homePage.autoscanOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.getAutoscanModeTimed().then(function (checked) {
          expect(checked).to.equal('true')
        })
        homePage.getAutoscanLevelBasic().then(function (checked) {
          expect(checked).to.equal('true')
        })
        homePage.getAutoscanRecursive().then(function (checked) {
          expect(checked).to.equal('true')
        })
        homePage.getAutoscanHidden().then(function (checked) {
          expect(checked).to.equal('true')
        })
        homePage.getAutoscanIntervalValue().then(function (value) {
          expect(value).to.equal('1800')
        })
        homePage.cancelAutoscan()
      })
    })
  })

  test.it('autoscan displays message when properly submitted to server', function () {
    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc')
    homePage.clickTrailAddAutoscan().then(function () {
      homePage.autoscanOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.submitAutoscan()

        homePage.autoscanOverlayDisplayed().then(function (displayed) {
          expect(displayed).to.be.false
        })

        homePage.getToastMessage().then(function (message) {
          expect(message).to.equal('Performing full scan: /Movies')
        })

        homePage.waitForToastClose().then(function (displayed) {
          expect(displayed).to.be.false
        })
      })
    })
  })

  test.it('an existing autoscan item loads edit overlay', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Playlists')
    homePage.clickAutoscanEdit(1).then(function () {
      homePage.autoscanOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
      })
    })
  })
})
