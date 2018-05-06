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
    await driver.get(mockWebServer + '/reset?testName=trail.spec.json');

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

