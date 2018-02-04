var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The menu bar', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(60000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=menu.spec.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/index.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('without logging in the database and filetype menus are disabled', function () {
    homePage.isDisabled('nav-db').then(function (disabled) {
      expect(disabled).to.be.true
    })

    homePage.isDisabled('nav-fs').then(function (disabled) {
      expect(disabled).to.be.true
    })
  })

  test.it('enables the database and filetype menus upon login', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()

    homePage.getDatabaseMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        expect(value).to.equal('nav-link')
      })
    })

    homePage.getFileSystemMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        expect(value).to.equal('nav-link')
      })
    })
  })

  test.it('loads the parent database container list when clicking Database', function () {
    homePage.clickMenu('nav-db').then(function () {
      homePage.treeItems().then(function (tree) {
        expect(tree.length).to.equal(6)
      })
    })
  })

  test.it('loads the parent filesystem container list when clicking Filesystem', function () {
    homePage.clickMenu('nav-fs').then(function () {
      homePage.treeItems().then(function (tree) {
        expect(tree.length).to.equal(19)
      })
    })
  })

  test.it('clears the tree when Home menu is clicked', function () {
    homePage.clickMenu('nav-home').then(function () {
      homePage.treeItems().then(function (tree) {
        expect(tree.length).to.equal(0)
      })
    })
  })
})
