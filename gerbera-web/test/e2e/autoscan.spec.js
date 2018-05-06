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
    await driver.get(mockWebServer + '/reset?testName=autoscan.spec.json');

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

  describe('The autoscan overlay', () => {

    it('a gerbera tree fs item container opens autoscan overlay when trail add autoscan is clicked', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.clickTrailAddAutoscan();

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;

      result = await homePage.getAutoscanModeTimed();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanLevelBasic();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanRecursive();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanHidden();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanIntervalValue();
      expect(result).to.equal('1800');

      await homePage.cancelAutoscan();
    });

    it('autoscan displays message when properly submitted to server', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      await homePage.clickTrailAddAutoscan();

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.submitAutoscan();

      result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.false;

      result = await homePage.getToastMessage();
      expect(result).to.equal('Performing full scan: /Movies');

      result = await homePage.waitForToastClose();
      expect(result).to.be.false;
    });

    it('an existing autoscan item loads edit overlay', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Playlists');
      await homePage.clickAutoscanEdit(1);

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;
    });
  });
});

