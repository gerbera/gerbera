const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Trail Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('trail.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera trail', () => {

    it('a gerbera tree item shows an add icon in the trail but not delete item', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      let result = await homePage.hasTrailAddIcon();
      expect(result).to.be.true;

      result = await homePage.hasTrailAddAutoscanIcon();
      expect(result).to.be.true;

      result = await homePage.hasTrailDeleteIcon();
      expect(result).to.be.false;
    });

    it('a gerbera tree db item container shows delete icon and add icon in the trail', async () =>{
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      let result = await homePage.hasTrailDeleteIcon();
      expect(result).to.be.true;

      result = await homePage.hasTrailAddIcon();
      expect(result).to.be.true;
    });
  });
});

