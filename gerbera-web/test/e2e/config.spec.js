const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Config Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('config_load.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera config', () => {

    it('a gerbera config item is changed', async () => {
      await homePage.clickMenu('nav-config');
      await driver.sleep(1000); // allow for load
      await homePage.showConfig('Server');

      let result = await homePage.editOverlayFieldValue('value__server_modelNumber_7_0');
      expect(result).to.equal('1.7.0');

      await homePage.setEditorOverlayField('value__server_modelNumber_7_0', '43');

      result = await homePage.editOverlayFieldValue('value__server_modelNumber_7_0');
      expect(result).to.equal('43');

      await homePage.clickTrail(1);

      result = await homePage.getToastMessage();
      expect(result).to.equal('Successfully saved 1 config items');
    });
  });
});

