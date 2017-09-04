var assert = require('assert')
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var LoginPage = require('./page/login.page')

test.describe('The login page', function () {
  var loginPage

  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=login.test.requests.json')

    loginPage = new LoginPage(driver)
    loginPage.get(mockWebServer + '/gerbera.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('hides the login form when no authentication is required', function () {
    loginPage.loginFields().then(function (fields) {
      for (var i = 0; i < fields.length; i++) {
        var field = fields[i]
        field.getAttribute('style').then(function (value) {
          assert.equal(value, 'display: none;')
        })
      }
    })
  })

  test.it('shows the login form when no session exists yet and accounts is required', function () {
    loginPage.get(mockWebServer + '/gerbera.html')
    loginPage.loginForm().then(function (form) {
      form.getAttribute('style').then(function (value) {
        assert.equal(value, '')
      })
    })
  })

  test.it('requires user name and password to submit the login form', function () {
    loginPage.password('')
    loginPage.username('')
    loginPage.submitLogin()
    loginPage.warning().then(function (warning) {
      assert.ok(warning.includes('Please enter username and password'))
    })
  })

  test.it('when successful login show logout button, and show form on logout', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
    loginPage.logout()
    loginPage.loginForm().then(function (form) {
      form.getAttribute('style').then(function (value) {
        assert.equal(value, '')
      })
    })
  })
})
