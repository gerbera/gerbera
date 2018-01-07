var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The gerbera toast', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=toast.spec.json')

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

  test.it('shows with max-width of screen', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTree('All Video')
    homePage.getToastElement().then(function (element) {
      element.getSize().then(function (size) {
        expect(size.width > 1200).to.be.true
      })
    })
  })
})
