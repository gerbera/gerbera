var assert = require('assert')
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The home page', function () {
  var loginPage, homePage

  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=home.test.requests.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    loginPage.get(mockWebServer + '/gerbera.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('without logging in the database and filetype menus are disabled', function () {
    homePage.getDatabaseMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        assert.equal(value, 'navbar-link disabled')
      })
    })

    homePage.getFileSystemMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        assert.equal(value, 'navbar-link disabled')
      })
    })
  })

  test.it('enables the database and filetype menus upon login', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()

    homePage.getDatabaseMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        assert.equal(value, 'navbar-link')
      })
    })

    homePage.getFileSystemMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        assert.equal(value, 'navbar-link')
      })
    })
  })

  test.it('disables the database and filetype menus upon logout', function () {
    loginPage.logout()

    homePage.getDatabaseMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        assert.equal(value, 'navbar-link disabled')
      })
    })

    homePage.getFileSystemMenu().then(function (menu) {
      menu.getAttribute('class').then(function (value) {
        assert.equal(value, 'navbar-link disabled')
      })
    })
  })

  test.it('loads the parent database container list when clicking Database', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db').then(function () {
      homePage.treeItems().then(function (tree) {
        assert.equal(tree.length, 6)
      })
    })
  })

  test.it('loads the parent filesystem container list when clicking Filesystem', function () {
    homePage.clickMenu('nav-fs').then(function () {
      homePage.treeItems().then(function (tree) {
        assert.equal(tree.length, 19)
      })
    })
  })

  test.it('clears the tree when Home menu is clicked', function () {
    homePage.clickMenu('nav-home').then(function () {
      homePage.treeItems().then(function (tree) {
        assert.equal(tree.length, 0)
      })
    })
  })

  test.it('loads the related items when tree item is clicked', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video').then(function () {
      homePage.items().then(function (items) {
        assert.equal(items.length, 11)
      })
    })
  })

  test.it('loads child tree items when tree item is expanded', function () {
    homePage.clickMenu('nav-db')
    homePage.expandTree('Video').then(function () {
      homePage.treeItemChildItems('Video').then(function (items) {
        assert.equal(items.length, 2)
      })
    })
  })
})
