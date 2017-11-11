var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The gerbera items', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=items.spec.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    driver.get(mockWebServer + '/disabled.html')
    driver.manage().deleteAllCookies()

    loginPage.get(mockWebServer + '/gerbera.html')
  })

  test.after(function () {
    driver.quit()
  })

  test.it('loads the related items when tree item is clicked', function () {
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()

    homePage.clickMenu('nav-db')
    homePage.clickTree('Video').then(function () {
      homePage.items().then(function (items) {
        expect(items.length).to.equal(13)
      })
    })
  })

  test.it('a gerbera item contains the edit icon', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.getItem(2).then(function (item) {
      homePage.hasEditIcon(item).then(function (has) {
        expect(has).to.be.true
      })
    })
  })

  test.it('a gerbera item contains the download icon', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.getItem(2).then(function (item) {
      homePage.hasDownloadIcon(item).then(function (has) {
        expect(has).to.be.true
      })
    })
  })

  test.it('a gerbera item contains the delete icon', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.getItem(2).then(function (item) {
      homePage.hasDeleteIcon(item).then(function (hasDelete) {
        expect(hasDelete).to.be.true
      })
    })
  })

  test.it('loads child tree items when tree item is expanded', function () {
    homePage.clickMenu('nav-db')
    homePage.expandTree('Video').then(function () {
      homePage.treeItemChildItems('Video').then(function (items) {
        expect(items.length).to.equal(2)
      })
    })
  })

  test.it('a gerbera file item does not contain any modify icons', function () {
    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc')
    homePage.getItem(2).then(function (item) {
      homePage.hasEditIcon(item).then(null, function (err) {
        expect(err.name).to.equal('NoSuchElementError')
      })

      homePage.hasDeleteIcon(item).then(null, function (err) {
        expect(err.name).to.equal('NoSuchElementError')
      })

      homePage.hasDownloadIcon(item).then(null, function (err) {
        expect(err.name).to.equal('NoSuchElementError')
      })
    })
  })

  test.it('a gerbera file item contains an add icon', function () {
    homePage.clickMenu('nav-fs')
    homePage.clickTree('etc')
    homePage.getItem(1).then(function (item) {
      homePage.hasAddIcon(item).then(function (hasAdd) {
        expect(hasAdd).to.be.true
      })
    })
  })

  test.it('the gerbera items contains a pager that retrieves pages of items', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Playlists')
    homePage.getPages().then(function (pages) {
      expect(pages).to.have.lengthOf(4)

      pages[2].click().then(function () {
        homePage.getItem(0).then(function (item) {
          item.getText().then(function (text) {
            expect(text).to.equal('Test25.mp4')
          })
        })
      })

    })
  })
})
