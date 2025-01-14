/* global process */
const { Builder } = require('selenium-webdriver');
const { suite } = require('selenium-webdriver/testing');
let chrome = require('selenium-webdriver/chrome');
const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;
const { argv } = require('yargs');
const Jimp = require('jimp');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

suite(() => {
  let loginPage, homePage, suffix;
  const DEFAULT_FOLDER_STORE = process.cwd() + argv.folderPath;
  console.log('Save path --> ' + DEFAULT_FOLDER_STORE);
  const test = argv.scheme;
  before(async () => {
    const chromeOptions = new chrome.Options();
    suffix = '.png';
    chromeOptions.addArguments(['--window-size=1440,1080', '--incognito']);
    if (test == 'dark') {
        chromeOptions.addArguments(['--force-dark-mode']);
        suffix = '_dark.png';
    }
    driver = new Builder()
      .forBrowser('chrome')
      .setChromeOptions(chromeOptions)
      .build();
    await driver.get(mockWebServer + '/reset?testName=default.json');
    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);
  });

  describe('The screenshot documentation spec takes screenshots', () => {

    beforeEach(async () => {
      await driver.get(mockWebServer + '/disabled.html');
      await driver.manage().deleteAllCookies();
      await driver.executeScript("localStorage.clear()");
      await loginPage.get(mockWebServer + '/index.html');
    });

    after(() => driver && driver.quit());

    it('for [main view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'main-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await loginPage.statsVisible();

      await loginPage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 680);
      image.write(fileName);
    });

    it('for [main view] with empty database', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'main-view-empty' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await loginPage.emptyVisible();

      await loginPage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 780);
      image.write(fileName);
    });

    it('for [login] field entry', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'login-field-entry' + suffix;
      await loginPage.username('gerbera');
      await loginPage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(420, 100, 600, 470);
      image.write(fileName);
    });

    it('for [menu bar]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'menubar' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();

      await loginPage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(0, 0, 1430, 80);
      image.write(fileName);
    });

    it('for [database view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'database-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 700);
      image.write(fileName);
    });

    it('for [database small grid view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'database-smallgrid-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.setSelectValue('gridSelect', 1);
      await homePage.setSelectValue('ippSelect', 0);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 450);
      image.write(fileName);
    });

    it('for [database large grid view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'database-largegrid-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.setSelectValue('gridSelect', 2);
      await homePage.setSelectValue('ippSelect', 10);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 500);
      image.write(fileName);
    });

    it('for [database single view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'database-single-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Photos');
      await homePage.setSelectValue('gridSelect', 3);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.write(fileName);
    });

    it('for [search view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'search-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-search');
      await homePage.clickTree('Photos');
      await homePage.setTextValue('searchQuery', 'upnp:class derivedfrom "object.item.audioItem.musicTack" and dc:title contains "wallaby"');
      await homePage.setTextValue('searchSort', 'upnp:date');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 500);
      image.write(fileName);
    });

    it('for [filesystem view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'filesystem-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.write(fileName);
    });

    it('for [clients view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'clients-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-clients');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 500);
      image.write(fileName);
    });

    it('for [config view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'config-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-config');
      await driver.sleep(500);
      await homePage.showConfig('Server (/server)');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO);
      image.crop(0, 0, 1280, 750);
      image.write(fileName);
    });

    it('for [items view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'items-view' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO).write(fileName);
    });

    it('for [edit item] from item list', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'edit-item' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.editItem(0);
      await driver.sleep(2000);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.crop(600, 140, 530, 950);
      image.write(fileName);
    });

    it('for [edit item] from item list with details', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'edit-item-details' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.editItem(0);
      await driver.sleep(2000);
      await homePage.showDetails();

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.crop(280, 50, 1160, 1150);
      image.write(fileName);
    });

    it('for [trail operations]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'trail-operations' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(860, 0, 570, 140);
      image.write(fileName);
    });

    it('for [trail filesystem operations]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'trail-fs-operations' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(860, 0, 570, 140);
      image.write(fileName);
    });

    it('for [edit autoscan] from filesystem list', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'edit-autoscan' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      await homePage.clickTrail(1);
      await driver.sleep(1000);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(380, 200, 675, 660);
      image.write(fileName);
    });

    it('for [edit autoscan] from filesystem list with details', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'edit-autoscan-details' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      await homePage.clickTrail(1);
      await driver.sleep(1000);
      await homePage.showAutoscanDetails();

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(235, 160, 960, 735);
      image.write(fileName);
    });

    it('for [edit tweak] from filesystem list', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'edit-tweak-details' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      await homePage.clickTrail(2);
      await driver.sleep(1000);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(290, 0, 860, 600);
      image.write(fileName);
    });

    it('for [trail config operations]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'trail-config-operations' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-config');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(860, 0, 570, 140);
      image.write(fileName);
    });

    it('for [item edit operations]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'item-operations' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.getItem(0);
      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(325, 125, 1115, 125);
      image.write(fileName);
    });

    it('for [toast message]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'toast-message' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.clickTree('All Video');

      await homePage.getToastElement();
      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(0, 990, 1440, 74);
      image.write(fileName);
    });

    it('for [task message]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'task-message' + suffix;
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.mockTaskMessage('Scan of /movies');
      await driver.sleep(1000);
      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, Jimp.AUTO);
      image.crop(0, 990, 1440, 74);
      image.write(fileName);
    });
  });
});
