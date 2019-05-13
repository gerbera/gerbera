const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Add Object Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('add.object.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);

    await loginPage.get(TestUtils.home());

    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The add new object', () => {
    it('a gerbera tree db item container adds new object with parent id', async () => {
      const itemCountBefore = 13;
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      let result = await homePage.items();
      expect(result.length).to.equal(itemCountBefore);

      await homePage.clickTrailAdd();

      result = await homePage.editOverlayDisplayed();
      expect(result).to.be.true;

      result = await homePage.editOverlayFieldValue('addParentId');
      expect(result).to.equal('7443');

      result = await homePage.editOverlayTitle();
      expect(result).to.equal('Add Item');

      await homePage.setEditorOverlayField('editTitle', 'A Container');
      await homePage.submitEditor();

      result = await homePage.items();
      expect(result.length).to.equal(itemCountBefore + 1);

      result = await homePage.getToastMessage();
      expect(result).to.equal('Successfully added object');
      await homePage.closeToast();
    });
  });
});
