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
    chromeOptions.addArguments(['--window-size=1280,1024']);
    driver = new Builder()
      .forBrowser('chrome')
      .setChromeOptions(chromeOptions)
      .build();
    await driver.get(mockWebServer + '/reset?testName=delete.item.spec.json');

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await driver.get(mockWebServer + '/disabled.html');
    await driver.manage().deleteAllCookies();
    await loginPage.get(mockWebServer + '/index.html');
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

      result = await homePage.waitForToastClose();
      expect(result).to.be.false;
    });
  });
});
