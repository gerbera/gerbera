const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Toast Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('toast.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera toast', () => {
    it('shows with max-width of screen', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      await homePage.clickTree('All Video');

      const width = await homePage.getToastElementWidth();
      expect(width > 1200, 'Width is: ' + width).to.be.true;
    });
  });
});