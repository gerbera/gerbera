const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Items Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('items.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera items', () => {
    it('loads the related items when tree item is clicked', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const result = await homePage.items();
      expect(result.length).to.equal(13);
    });

    it('a gerbera item contains the edit icon', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const item = await homePage.getItem(2);
      const result = await homePage.hasEditIcon(item);
      expect(result).to.be.true;
    });

    it('a gerbera item contains the download icon', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const item = await homePage.getItem(2);
      const result = await homePage.hasDownloadIcon(item);
      expect(result).to.be.true;
    });

    it('a gerbera item contains the delete icon', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const item = await homePage.getItem(2);
      const result = await homePage.hasDeleteIcon(item);
      expect(result).to.be.true;
    });

    it('loads child tree items when tree item is expanded', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.expandTree('Video');
      const items = await homePage.treeItemChildItems('Video');
      expect(items.length).to.equal(2);
    });

    it('a gerbera file item does not contain any modify icons', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      const item = await homePage.getItem(2);

      let result = await homePage.hasEditIcon(item);
      expect(result).to.be.false;

      result = await homePage.hasDeleteIcon(item);
      expect(result).to.be.false;

      result = await homePage.hasDownloadIcon(item);
      expect(result).to.be.false;
    });

    it('a gerbera file item contains an add icon', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      const item = await homePage.getItem(1);
      const result = await homePage.hasAddIcon(item);
      expect(result).to.be.true;
    });

    it('the gerbera items contains a pager that retrieves pages of items', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Playlists');

      const pages = await homePage.getPages();
      expect(pages).to.have.lengthOf(4);

      await pages[2].click();

      const item = await homePage.getItem(0);
      const text = await item.getText();
      expect(text).to.equal('Test25.mp4');
    });
  });
});