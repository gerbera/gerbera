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
    await driver.get(mockWebServer + '/reset?testName=logout.spec.json');

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await driver.get(mockWebServer + '/disabled.html');
    await driver.manage().deleteAllCookies();
    await loginPage.get(mockWebServer + '/index.html');
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
