var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The invalid session error handler', function () {
  var loginPage, homePage

  this.slow(7000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=invalid.session.spec.json')

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

  test.it('displays a popup when invalid session is detected from server', function () {
    homePage.clickMenu('nav-db')
    homePage.getToastMessage().then(function (message) {
      expect(message).to.equal('invalid session id')
    })
  })
})
