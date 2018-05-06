const {expect} = require('chai');
const {Builder} = require('selenium-webdriver');
const {suite} = require('selenium-webdriver/testing');
let chrome = require('selenium-webdriver/chrome');
const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

suite(() => {
  let loginPage, homePage;

  before(async () => {
    const chromeOptions = new chrome.Options();
    chromeOptions.addArguments(['--window-size=1280,1024']);
    driver = new Builder()
      .forBrowser('chrome')
      .setChromeOptions(chromeOptions)
      .build();
    await driver.get(mockWebServer + '/reset?testName=overlay.test.json');

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await driver.get(mockWebServer + '/disabled.html');
    await driver.manage().deleteAllCookies();
    await loginPage.get(mockWebServer + '/index.html');
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The overlay page', () => {

    afterEach(async () => {
      await homePage.cancelEdit();
    });

    it('shows the edit overlay when edit is clicked and contains details', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.editItem(0);

      const isDisplayed = await homePage.editOverlayDisplayed();
      expect(isDisplayed).to.be.true;

      let value = await homePage.editOverlayFieldValue('editObjectType');
      expect(value).to.equal('item');

      value = await homePage.editOverlayFieldValue('editLocation');
      expect(value).to.equal('/folder/location/Test.mp4');

      value = await homePage.editOverlayFieldValue('editClass');
      expect(value).to.equal('object.item.videoItem');

      value = await homePage.editOverlayFieldValue('editDesc');
      expect(value).to.equal('A description');

      value = await homePage.editOverlayFieldValue('editMime');
      expect(value).to.equal('video/mp4');

      value = await homePage.editOverlayFieldValue('objectId');
      expect(value).to.equal('39479');

      const text = await homePage.editOverlaySubmitText();
      expect(text).to.equal('Save Item');
    });

    it('on click of db add item, shows the modal', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.clickTrailAdd();

      let result = await homePage.editOverlayDisplayed();
      expect(result).to.be.true;

      result = await homePage.editOverlayTitle();
      expect(result).to.equal('Add Item');

      result = await homePage.editOverlaySubmitText();
      expect(result).to.equal('Add Item');
    });

    it('selected object defaults class when selected', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.clickTrailAdd();

      let result = await homePage.editOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.selectObjectType('item');

      result = await homePage.editOverlayFieldValue('editClass');
      expect(result).to.equal('object.item');
    });
  });

  describe('The overlay item type selection', () => {

    afterEach(async () => {
      await homePage.cancelEdit();
    });

    const itemSelectionTest = async (itemType, assertClass, fieldAssertions) => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.clickTrailAdd();

      let result = await homePage.editOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.selectObjectType(itemType);

      const objectClass = await homePage.editOverlayFieldValue('editClass');
      expect(objectClass, 'Object Class').to.equal(assertClass);

      for (let i = 0; i < fieldAssertions.length; i++) {
        const fieldAssert = fieldAssertions[i];
        const isDisplayed = await homePage.editorOverlayFieldDisplayed(fieldAssert.id);
        expect(isDisplayed, 'Field: ' + fieldAssert.id).to.equal(fieldAssert.visible);
      }
    };

    it('only shows type, title and class when adding container type', async () => {
      await itemSelectionTest('container', 'object.container', [
        { id : 'editObjectType',   visible : true},
        { id : 'editTitle',        visible : true},
        { id : 'editClass',        visible : true},
        { id : 'editLocation',     visible : false},
        { id : 'editDesc',         visible : false},
        { id : 'editMime',         visible : false},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : false}
      ]);
    });

    it('only shows title, location, class, desc, mime when adding item type', async () => {
      await itemSelectionTest('item', 'object.item', [
        { id : 'editObjectType',   visible : true},
        { id : 'editTitle',        visible : true},
        { id : 'editClass',        visible : true},
        { id : 'editLocation',     visible : true},
        { id : 'editDesc',         visible : true},
        { id : 'editMime',         visible : true},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : false}
      ]);
    });

    it('only shows title, location, class, desc, mime, actionScript, state when adding activeItem type', async () => {
      await itemSelectionTest('active_item', 'object.item.activeItem', [
        { id : 'editObjectType',   visible : true},
        { id : 'editTitle',        visible : true},
        { id : 'editClass',        visible : true},
        { id : 'editLocation',     visible : true},
        { id : 'editDesc',         visible : true},
        { id : 'editMime',         visible : true},
        { id : 'editActionScript', visible : true},
        { id : 'editState',        visible : true},
        { id : 'editProtocol',     visible : false}
      ]);
    });

    it('only shows title, location, class, desc, mime, protocol when adding external_url type', async () => {
      await itemSelectionTest('external_url', 'object.item', [
        { id : 'editObjectType',   visible : true},
        { id : 'editTitle',        visible : true},
        { id : 'editClass',        visible : true},
        { id : 'editLocation',     visible : true},
        { id : 'editDesc',         visible : true},
        { id : 'editMime',         visible : true},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : true}
      ]);
      const editProtocol = await homePage.editOverlayFieldValue('editProtocol');
      expect(editProtocol).to.equal('http-get');
    });

    it('only shows title, location, class, desc, mime when adding internal_url type', async () => {
      await itemSelectionTest('internal_url', 'object.item', [
        { id : 'editObjectType',   visible : true},
        { id : 'editTitle',        visible : true},
        { id : 'editClass',        visible : true},
        { id : 'editLocation',     visible : true},
        { id : 'editDesc',         visible : true},
        { id : 'editMime',         visible : true},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : false}
      ]);
    });
  });

  describe('The overlay load item', () => {

    afterEach(async () => {
      await homePage.cancelEdit();
    });

    const itemLoadTest = async (itemNumber, fieldAssertions) => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      if (itemNumber === undefined) {
        await homePage.clickTrailEdit();
      } else {
        await homePage.editItem(itemNumber);
      }

      let result = await homePage.editOverlayDisplayed();
      expect(result).to.be.true;

      for (let i = 0; i < fieldAssertions.length; i++) {
        const fieldAssert = fieldAssertions[i];
        const isDisplayed = await homePage.editorOverlayFieldDisplayed(fieldAssert.id);
        expect(isDisplayed, 'Field: ' + fieldAssert.id).to.equal(fieldAssert.visible);
        if(fieldAssert.value){
          const value = await homePage.editOverlayFieldValue(fieldAssert.id);
          expect(value, 'Value: ' + fieldAssert.id).to.equal(fieldAssert.value);
        }
      }
    };

    it('loads a container item to edit with correct fields populated', async () => {
      await itemLoadTest(undefined, [
        { id : 'editObjectType',   visible : true, value : 'container'},
        { id : 'editTitle',        visible : true, value : 'container title'},
        { id : 'editClass',        visible : true, value : 'object.container'},
        { id : 'editLocation',     visible : false},
        { id : 'editDesc',         visible : false},
        { id : 'editMime',         visible : false},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : false}
      ]);
    });

    it('loads an active item to edit with correct fields populated', async () => {
      await itemLoadTest(10, [
        { id : 'editObjectType',   visible : true, value : 'active_item'},
        { id : 'editTitle',        visible : true, value : 'Test Active Item'},
        { id : 'editClass',        visible : true, value : 'object.item.activeItem'},
        { id : 'editLocation',     visible : true, value : '/home/test.txt'},
        { id : 'editDesc',         visible : true, value : 'test'},
        { id : 'editMime',         visible : true, value : 'text/plain'},
        { id : 'editActionScript', visible : true, value : '/home/echoText.sh'},
        { id : 'editState',        visible : true, value : 'test-state'},
        { id : 'editProtocol',     visible : false}
      ]);
    });

    it('loads an external url to edit with correct fields populated', async () => {
      await itemLoadTest(11, [
        { id : 'editObjectType',   visible : true, value : 'external_url'},
        { id : 'editTitle',        visible : true, value : 'title'},
        { id : 'editClass',        visible : true, value : 'object.item'},
        { id : 'editLocation',     visible : true, value : 'http://localhost'},
        { id : 'editDesc',         visible : true, value : 'description'},
        { id : 'editMime',         visible : true, value : 'text/plain'},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : true, value : 'http-get'}
      ]);
    });

    it('loads an simple item to edit with correct fields populated', async () => {
      await itemLoadTest(0, [
        { id : 'editObjectType',   visible : true, value : 'item'},
        { id : 'editTitle',        visible : true, value : 'Test.mp4'},
        { id : 'editClass',        visible : true, value : 'object.item.videoItem'},
        { id : 'editLocation',     visible : true, value : '/folder/location/Test.mp4'},
        { id : 'editDesc',         visible : true, value : 'A description'},
        { id : 'editMime',         visible : true, value : 'video/mp4'},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : false}
      ]);
    });

    it('loads an internal url to edit with correct fields populated', async () => {
      await itemLoadTest(12, [
        { id : 'editObjectType',   visible : true, value : 'internal_url'},
        { id : 'editTitle',        visible : true, value : 'title'},
        { id : 'editClass',        visible : true, value : 'object.item'},
        { id : 'editLocation',     visible : true, value : './test'},
        { id : 'editDesc',         visible : true, value : 'description'},
        { id : 'editMime',         visible : true, value : 'text/plain'},
        { id : 'editActionScript', visible : false},
        { id : 'editState',        visible : false},
        { id : 'editProtocol',     visible : true, value : 'http-get'}
      ]);
    });
  });
});
