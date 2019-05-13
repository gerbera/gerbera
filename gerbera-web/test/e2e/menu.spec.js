const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Menu Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('menu.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
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

