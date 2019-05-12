const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Delete Item Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('delete.item.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera delete item', () => {
    it('deletes item from the list when delete is clicked', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      await homePage.deleteItemFromList(0);

      let result = await homePage.items();
      expect(result.length).to.equal(10);

      result = await homePage.getToastMessage();
      expect(result).to.equal('Successfully removed item');

      await homePage.closeToast();
    });
  });
});
