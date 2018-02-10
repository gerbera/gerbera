/* global process */
var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var argv = require('yargs').argv
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var LoginPage = require('./page/login.page')
var HomePage = require('./page/home.page')

test.describe('The screenshot documentation spec takes screenshots', function () {
  var loginPage, homePage
  var DEFAULT_FOLDER_STORE = process.cwd() + argv.folderPath
  console.log('Save path ---> ' + DEFAULT_FOLDER_STORE)
  var Jimp = require("jimp")

  this.slow(5000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=default.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)
  })

  test.beforeEach(function () {
    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/index.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('for [login] field entry', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'login-field-entry.png'
    loginPage.username('gerbera')
    loginPage.takeScreenshot(fileName).then(function () {
      Jimp.read(fileName).then(function (image) {
        image.resize(1280, 1024)
        image.crop(0, 0, 1270, 100).write(fileName)
        done()
      });
    })
  })

  test.it('for [menu bar]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'menubar.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()
    loginPage.takeScreenshot(fileName).then(function () {
      Jimp.read(fileName).then(function (image) {
        image.resize(1280, 1024)
        image.crop(0, 0, 1270, 100).write(fileName)
        done()
      });
    })
  })

  test.it('for [database view]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'database-view.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video').then(function () {
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1024, Jimp.AUTO).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [filesystem view]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'filesystem-view.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc').then(function () {
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1024, Jimp.AUTO).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [items view]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'items-view.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video').then(function () {
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1024, Jimp.AUTO).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [edit item] from item list', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'edit-item.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.editItem(0).then(function () {
      driver.sleep(1000)
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1280, 1024)
          image.crop(300, 0, 656, 899).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [trail operations]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'trail-operations.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video').then(function () {
      homePage.hover('.grb-trail-edit')
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1280, 1024)
          image.crop(700,0, 570, 130).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [item edit operations]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'item-operations.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.getItem(0).then(function (item) {
      homePage.hover('.grb-item-edit')
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1280, 1024)
          image.crop(525, 125, 745, 125).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [toast message]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'toast-message.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTree('All Video')
    homePage.getToastElement().then(function (element) {
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1024, Jimp.AUTO).write(fileName)
          done()
        });
      })
    })
  })

  test.it('for [task message]', function (done) {
    var fileName = DEFAULT_FOLDER_STORE + 'task-message.png'
    loginPage.username('user')
    loginPage.password('pwd')
    loginPage.submitLogin()

    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc')
    homePage.mockTaskMessage('Performing scan of /movies').then(function () {
      driver.sleep(1000)
      homePage.takeScreenshot(fileName).then(function () {
        Jimp.read(fileName).then(function (image) {
          image.resize(1024, Jimp.AUTO).write(fileName)
          done()
        });
      })
    })
  })
})
