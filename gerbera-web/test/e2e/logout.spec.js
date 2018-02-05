var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The logout action', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(60000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=logout.spec.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/index.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('disables the database and filetype menus upon logout', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()

    loginPage.logout()

    homePage.isDisabled('nav-db').then(function (disabled) {
      expect(disabled).to.be.true
    })

    homePage.isDisabled('nav-fs').then(function (disabled) {
      expect(disabled).to.be.true
    })
  })
})
