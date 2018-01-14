var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The gerbera delete item', function () {
  var loginPage, homePage

  this.slow(8000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=delete.item.spec.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/index.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('deletes item from the list when delete is clicked', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.deleteItemFromList(0).then(function (item) {
      homePage.items().then(function (items) {
        expect(items.length).to.equal(10)
      })

      homePage.getToastMessage().then(function (message) {
        expect(message).to.equal('Successfully deleted item')
      })

      homePage.waitForToastClose().then(function (displayed) {
        expect(displayed).to.be.false
      })
    })
  })
})
