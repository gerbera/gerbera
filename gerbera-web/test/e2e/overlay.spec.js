var chai = require('chai')
var expect = chai.expect
var webdriver = require('selenium-webdriver')
var test = require('selenium-webdriver/testing')
var driver
var mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port

require('chromedriver')

var HomePage = require('./page/home.page')
var LoginPage = require('./page/login.page')

test.describe('The overlay page', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=overlay.test.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    loginPage.get(mockWebServer + '/index.html')
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
  })

  test.after(function () {
    driver.quit()
  })

  test.afterEach(function () {
    homePage.cancelEdit()
  })

  test.it('shows the edit overlay when edit is clicked and contains details', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.editItem(0).then(function (item) {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.editOverlayFieldValue('editObjectType').then(function (value) {
          expect(value).to.equal('item')
        })
        homePage.editOverlayFieldValue('editLocation').then(function (value) {
          expect(value).to.equal('/folder/location/Test.mp4')
        })
        homePage.editOverlayFieldValue('editClass').then(function (value) {
          expect(value).to.equal('object.item.videoItem')
        })
        homePage.editOverlayFieldValue('editDesc').then(function (value) {
          expect(value).to.equal('A description')
        })
        homePage.editOverlayFieldValue('editMime').then(function (value) {
          expect(value).to.equal('video/mp4')
        })
        homePage.editOverlayFieldValue('objectId').then(function (value) {
          expect(value).to.equal('39479')
        })
        homePage.editOverlaySubmitText().then(function (text) {
          expect(text).to.equal('Save Item')
        })
      })
    })
  })

  test.it('on click of db add item, shows the modal', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true

        homePage.editOverlayTitle().then(function (title) {
          expect(title).to.equal('Add Item')
        })
        homePage.editOverlaySubmitText().then(function (text) {
          expect(text).to.equal('Add Item')
        })
      })
    })
  })

  test.it('selected object defaults class when selected', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.selectObjectType('item').then(function () {
          homePage.editOverlayFieldValue('editClass').then(function (value) {
            expect(value).to.equal('object.item')
          })
        })
      })
    })
  })
})

test.describe('The overlay item type selection', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=overlay.test.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    loginPage.get(mockWebServer + '/index.html')
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
  })

  test.after(function () {
    driver.quit()
  })

  test.afterEach(function () {
    homePage.cancelEdit()
  })

  test.it('only shows type, title and class when adding container type', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.selectObjectType('container').then(function () {
          homePage.editOverlayFieldValue('editClass').then(function (value) {
            expect(value).to.equal('object.container')
          })
          homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
        })
      })
    })
  })

  test.it('only shows title, location, class, desc, mime when adding item type', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.selectObjectType('item').then(function () {
          homePage.editOverlayFieldValue('editClass').then(function (value) {
            expect(value).to.equal('object.item')
          })
          homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
        })
      })
    })
  })

  test.it('only shows title, location, class, desc, mime, actionScript, state when adding activeItem type', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.selectObjectType('active_item').then(function () {
          homePage.editOverlayFieldValue('editClass').then(function (value) {
            expect(value).to.equal('object.item.activeItem')
          })
          homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
        })
      })
    })
  })

  test.it('only shows title, location, class, desc, mime, protocol when adding external_url type', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.selectObjectType('external_url').then(function () {
          homePage.editOverlayFieldValue('editClass').then(function (value) {
            expect(value).to.equal('object.item')
          })
          homePage.editOverlayFieldValue('editProtocol').then(function (value) {
            expect(value).to.equal('http-get')
          })
          homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
        })
      })
    })
  })

  test.it('only shows title, location, class, desc, mime when adding internal_url type', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailAdd().then(function () {
      homePage.editOverlayDisplayed().then(function (displayed) {
        expect(displayed).to.be.true
        homePage.selectObjectType('internal_url').then(function () {
          homePage.editOverlayFieldValue('editClass').then(function (value) {
            expect(value).to.equal('object.item')
          })
          homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
            expect(value).to.be.true
          })
          homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
          homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
            expect(value).to.be.false
          })
        })
      })
    })
  })
})

test.describe('The overlay load item', function () {
  var loginPage, homePage

  this.slow(5000)
  this.timeout(15000)

  test.before(function () {
    driver = new webdriver.Builder()
      .forBrowser('chrome')
      .build()

    driver.manage().window().setSize(1280, 1024)
    driver.get(mockWebServer + '/reset?testName=overlay.test.json')

    loginPage = new LoginPage(driver)
    homePage = new HomePage(driver)

    loginPage.get(mockWebServer + '/index.html')
    loginPage.password('pwd')
    loginPage.username('user')
    loginPage.submitLogin()
  })

  test.after(function () {
    driver.quit()
  })

  test.afterEach(function () {
    homePage.cancelEdit()
  })

  test.it('loads a container item to edit with correct fields populated', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.clickTrailEdit().then(function () {
      homePage.editOverlayFieldValue('editObjectType').then(function (value) {
        expect(value).to.equal('container')
      })
      homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editTitle').then(function (value) {
        expect(value).to.equal('container title')
      })
      homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editClass').then(function (value) {
        expect(value).to.equal('object.container')
      })
      homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
    })
  })

  test.it('loads an active item to edit with correct fields populated', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.editItem(10).then(function () {
      homePage.editOverlayFieldValue('editObjectType').then(function (value) {
        expect(value).to.equal('active_item')
      })
      homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editTitle').then(function (value) {
        expect(value).to.equal('Test Active Item')
      })
      homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editClass').then(function (value) {
        expect(value).to.equal('object.item.activeItem')
      })
      homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editDesc').then(function (value) {
        expect(value).to.equal('test')
      })
      homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editMime').then(function (value) {
        expect(value).to.equal('text/plain')
      })
      homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editLocation').then(function (value) {
        expect(value).to.equal('/home/test.txt')
      })
      homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editActionScript').then(function (value) {
        expect(value).to.equal('/home/echoText.sh')
      })
      homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editState').then(function (value) {
        expect(value).to.equal('test-state')
      })
      homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
    })
  })

  test.it('loads an external url to edit with correct fields populated', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.editItem(11).then(function () {
      homePage.editOverlayFieldValue('editObjectType').then(function (value) {
        expect(value).to.equal('external_url')
      })
      homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editTitle').then(function (value) {
        expect(value).to.equal('title')
      })
      homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editClass').then(function (value) {
        expect(value).to.equal('object.item')
      })
      homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editDesc').then(function (value) {
        expect(value).to.equal('description')
      })
      homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editMime').then(function (value) {
        expect(value).to.equal('text/plain')
      })
      homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editLocation').then(function (value) {
        expect(value).to.equal('http://localhost')
      })
      homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editProtocol').then(function (value) {
        expect(value).to.equal('http-get')
      })
    })
  })

  test.it('loads an simple item to edit with correct fields populated', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.editItem(0).then(function () {
      homePage.editOverlayFieldValue('editObjectType').then(function (value) {
        expect(value).to.equal('item')
      })
      homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editTitle').then(function (value) {
        expect(value).to.equal('Test.mp4')
      })
      homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editClass').then(function (value) {
        expect(value).to.equal('object.item.videoItem')
      })
      homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editDesc').then(function (value) {
        expect(value).to.equal('A description')
      })
      homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editMime').then(function (value) {
        expect(value).to.equal('video/mp4')
      })
      homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editLocation').then(function (value) {
        expect(value).to.equal('/folder/location/Test.mp4')
      })
      homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
    })
  })

  test.it('loads an internal url to edit with correct fields populated', function () {
    homePage.clickMenu('nav-db')
    homePage.clickTree('Video')
    homePage.editItem(12).then(function () {
      homePage.editOverlayFieldValue('editObjectType').then(function (value) {
        expect(value).to.equal('internal_url')
      })
      homePage.editorOverlayField('editObjectType').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editTitle').then(function (value) {
        expect(value).to.equal('title')
      })
      homePage.editorOverlayField('editTitle').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editClass').then(function (value) {
        expect(value).to.equal('object.item')
      })
      homePage.editorOverlayField('editClass').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editDesc').then(function (value) {
        expect(value).to.equal('description')
      })
      homePage.editorOverlayField('editDesc').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editMime').then(function (value) {
        expect(value).to.equal('text/plain')
      })
      homePage.editorOverlayField('editMime').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editLocation').then(function (value) {
        expect(value).to.equal('./test')
      })
      homePage.editorOverlayField('editLocation').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editorOverlayField('editActionScript').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editState').isDisplayed().then(function (value) {
        expect(value).to.be.false
      })
      homePage.editorOverlayField('editProtocol').isDisplayed().then(function (value) {
        expect(value).to.be.true
      })
      homePage.editOverlayFieldValue('editProtocol').then(function (value) {
        expect(value).to.equal('http-get')
      })
    })
  })
})
