var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var LoginPage = require('./page/login.page')

test.describe('The login action', function () {
  var loginPage

  this.slow(5000)
  this.timeout(60000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=login.test.requests.json')

    loginPage = new LoginPage(driver)
  })

  test.beforeEach(function () {
    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/index.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('hides the login form when no authentication is required', function () {
    loginPage.loginFields().then(function (fields) {
      for (var i = 0; i < fields.length; i++) {
        var field = fields[i]
        field.getAttribute('style').then(function (value) {
          expect(value).to.equal('display: none;')
        })
      }
    })
  })

  test.it('shows the login form when no session exists yet and accounts is required', function () {
    loginPage.loginForm().then(function (form) {
      form.getAttribute('style').then(function (value) {
        expect(value).to.equal('')
      })
    })
  })

  test.it('requires user name and password to submit the login form', function () {
    loginPage.password('')
    loginPage.username('')
    loginPage.submitLogin()
    driver.sleep(4000)
    loginPage.getToastMessage().then(function (message) {
      expect(message).to.equal('Please enter username and password')
    })
    loginPage.waitForToastClose().then(function (displayed) {
      expect(displayed).to.be.false
    })
  })

  test.it('when successful login show logout button, and show form on logout', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
    loginPage.logout()
    loginPage.loginForm().then(function (form) {
      form.getAttribute('style').then(function (value) {
        expect(value).to.equal('')
      })
    })
  })

  test.it('hides menu, hides login and shows message when UI is disabled', function () {
    loginPage.getToastMessage().then(function (message) {
      expect(message).to.equal('The UI is disabled in the configuration file. See README.')
    })
    loginPage.waitForToastClose().then(function (displayed) {
      expect(displayed).to.be.false
    })
    loginPage.menuList().then(function (menuList) {
      menuList.getAttribute('style').then(function (value) {
        expect(value).to.equal('display: none;')
      })
    })
    loginPage.loginFields().then(function (fields) {
      for (var i = 0; i < fields.length; i++) {
        var field = fields[i]
        field.getAttribute('style').then(function (value) {
          expect(value).to.equal('display: none;')
        })
      }
    })
  })

  test.it('when session expires reloads the page and lets user login again.', function () {
    driver.sleep(1000) // allow fields to load
    loginPage.loginFields().then(function (fields) {
      for (var i = 0; i < fields.length; i++) {
        var field = fields[i]
        field.getAttribute('style').then(function (value) {
          expect(value).to.equal('')
        })
      }
    })

    loginPage.getCookie('SID').then(function (sid) {
      expect(sid).to.be.null
    })
  })
})
