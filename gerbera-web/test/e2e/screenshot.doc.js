/* global process */
const {expect} = require('chai');
const {Builder} = require('selenium-webdriver');
const {suite} = require('selenium-webdriver/testing');
let chrome = require('selenium-webdriver/chrome');
const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;
const {argv} = require('yargs');
const Jimp = require('jimp');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

suite(() => {
  let loginPage, homePage;
  const DEFAULT_FOLDER_STORE = process.cwd() + argv.folderPath;
  console.log('Save path --> ' + DEFAULT_FOLDER_STORE);

  before(async () => {
    const chromeOptions = new chrome.Options();
    chromeOptions.addArguments(['--window-size=1440,1080']);
    driver = new Builder()
      .forBrowser('chrome')
      .setChromeOptions(chromeOptions)
      .build();
    await driver.get(mockWebServer + '/reset?testName=default.json');

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);
  });

  describe('The screenshot documentation spec takes screenshots', () => {

    beforeEach(async() => {
      await driver.get(mockWebServer + '/disabled.html');
      await driver.manage().deleteAllCookies();
      await loginPage.get(mockWebServer + '/index.html');
    });

    after(() => driver && driver.quit());

    it('for [login] field entry', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'login-field-entry.png';
      await loginPage.username('gerbera');
      await loginPage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, 1080);
      image.crop(0, 0, 1430, 100).write(fileName);
    });

    it('for [menu bar]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'menubar.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();

      await loginPage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, 1080);
      image.crop(0, 0, 1430, 100).write(fileName);
    });

    it('for [database view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'database-view.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO).write(fileName);
    });

    it('for [filesystem view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'filesystem-view.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO).write(fileName);
    });

    it('for [items view]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'items-view.png';
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
      const fileName = DEFAULT_FOLDER_STORE + 'edit-item.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.editItem(0);
      await driver.sleep(1000);

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, 1080);
      image.crop(390, 0, 656, 899).write(fileName);
    });

    it('for [trail operations]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'trail-operations.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, 1080);
      image.crop(860, 0, 570, 140).write(fileName);
    });

    it('for [item edit operations]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'item-operations.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.getItem(0);
      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1440, 1080);
      image.crop(325, 125, 1135, 125).write(fileName);
    });

    it('for [toast message]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'toast-message.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.clickTree('All Video');

      await homePage.getToastElement();
      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO).write(fileName);
    });

    it('for [task message]', async () => {
      const fileName = DEFAULT_FOLDER_STORE + 'task-message.png';
      await loginPage.username('user');
      await loginPage.password('pwd');
      await loginPage.submitLogin();
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.mockTaskMessage('Performing scan of /movies');
      await driver.sleep(1000);
      await homePage.takeScreenshot(fileName);

      const image = await Jimp.read(fileName);
      image.resize(1280, Jimp.AUTO).write(fileName);
    });
  });
});

