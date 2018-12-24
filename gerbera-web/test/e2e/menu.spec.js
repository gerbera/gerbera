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
    chromeOptions.addArguments(['--headless', '--window-size=1280,1024']);
    driver = new Builder()
      .forBrowser('chrome')
      .setChromeOptions(chromeOptions)
      .build();
    await driver.get(mockWebServer + '/reset?testName=menu.spec.json');

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await driver.get(mockWebServer + '/disabled.html');
    await driver.manage().deleteAllCookies();
    await loginPage.get(mockWebServer + '/index.html');
  });

  after(() => driver && driver.quit());

  describe('The menu bar', () => {
    it('without logging in the database and filetype menus are disabled', async () => {
      let disabled = await homePage.isDisabled('nav-db');
      expect(disabled).to.be.true;

      disabled = await homePage.isDisabled('nav-fs');
      expect(disabled).to.be.true;
    });

    it('enables the database and filetype menus upon login', async () => {
      await loginPage.password('pwd');
      await loginPage.username('user');
      await loginPage.submitLogin();

      let menu = await homePage.getDatabaseMenu();
      let menuClass = await menu.getAttribute('class');
      expect(menuClass).to.equal('nav-link');

      menu = await homePage.getFileSystemMenu();
      menuClass = await menu.getAttribute('class');
      expect(menuClass).to.equal('nav-link');
    });

    it('loads the parent database container list when clicking Database', async () => {
      await homePage.clickMenu('nav-db');
      const tree = await homePage.treeItems();
      expect(tree.length).to.equal(6);
    });

    it('loads the parent filesystem container list when clicking Filesystem', async () => {
      await homePage.clickMenu('nav-fs');
      const tree = await homePage.treeItems();
      expect(tree.length).to.equal(19);
    });

    it('clears the tree when Home menu is clicked', async () => {
      await homePage.clickMenu('nav-home');
      const tree = await homePage.treeItems();
      expect(tree.length).to.equal(0);
    });

    it('shows the friendly name in the Home menu', async () => {
      const homeMenu = await homePage.getHomeMenu();
      const text = await homeMenu.getText();
      expect(text).to.equal('Home [Gerbera Media Server]')
    });

    it('shows the friendly name in the document title', async () => {
      const title = await homePage.getTitle();
      expect(title).to.equal('Gerbera Media Server | Gerbera Media Server')
    });

    it('loads the parent database container list when clicking Database Icon', async () => {
      await homePage.clickMenuIcon('nav-db');
      const tree = await homePage.treeItems();
      expect(tree.length).to.equal(6);
    });
  });
});

