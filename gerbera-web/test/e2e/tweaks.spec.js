const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Tweaks Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('tweaks.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The directory tweaks overlay', () => {

    it('a gerbera tree fs item container opens dirtweaks overlay when trail directory tweaks is clicked', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      await homePage.clickTrailTweak();

      let result = await homePage.tweaksOverlayDisplayed();
      expect(result).to.be.true;

      result = await homePage.editOverlayFieldAttribute('dirTweakInherit', 'checked');
      expect(result).to.equal('true');

      result = await homePage.editOverlayFieldAttribute('dirTweakSymLinks', 'checked');
      expect(result).to.equal('true');

      result = await homePage.editOverlayFieldAttribute('dirTweakRecursive', 'checked');
      expect(result).to.equal(null);

      result = await homePage.editOverlayFieldAttribute('dirTweakCaseSens', 'checked');
      expect(result).to.equal(null);

      result = await homePage.editOverlayFieldAttribute('dirTweakHidden', 'checked');
      expect(result).to.equal('true');

      result = await homePage.editOverlayFieldValue('dirTweakFanArt');
      expect(result).to.equal('cover.jpg');

      result = await homePage.editOverlayFieldValue('dirTweakSubtitle');
      expect(result).to.equal('');

      result = await homePage.editOverlayFieldValue('dirTweakResource');
      expect(result).to.equal('');

      await homePage.cancelTweaks();
    });

    it('tweaks displays message when properly submitted to server', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      await homePage.clickTrailTweak();

      let result = await homePage.tweaksOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.submitTweaks();

      result = await homePage.tweaksOverlayDisplayed();
      expect(result).to.be.false;

      result = await homePage.getToastMessage();
      expect(result).to.equal('Successfully submitted directory tweak');

      await homePage.closeToast();
    });
  });
});

