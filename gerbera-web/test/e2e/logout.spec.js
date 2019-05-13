const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Logout Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('logout.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
  });

  after(() => driver && driver.quit());

  describe('The logout action', () => {

    it('disables the database and filetype menus upon logout', async () => {
      await loginPage.password('pwd');
      await loginPage.username('user');
      await loginPage.submitLogin();

      await loginPage.logout();

      let disabled = await homePage.isDisabled('nav-db');
      expect(disabled).to.be.true

      disabled = await homePage.isDisabled('nav-fs');
      expect(disabled).to.be.true
    });
  });
});
