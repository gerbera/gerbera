const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const LoginPage = require('./page/login.page');

describe('Login Suite', () => {
  let loginPage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('login.test.requests.json', driver);
    loginPage = new LoginPage(driver);
  });

  after(() => driver && driver.quit());

  describe('The login action', () => {

    beforeEach(async () => {
      await TestUtils.resetCookies(driver);
      await loginPage.get(TestUtils.home());
    });

    it('hides the login form when no authentication is required', async () => {
      const fields = await loginPage.loginFields();
      for (let i = 0; i < fields.length; i++) {
        const field = fields[i];
        const style = await field.getAttribute('style');
        expect(style).to.equal('display: none;');
      }
    });

    it('shows the login form when no session exists yet and accounts is required', async () => {
      const form = await loginPage.loginForm();
      const style = await form.getAttribute('style');
      expect(style).to.equal('');
    });

    it('requires user name and password to submit the login form', async () => {
      await loginPage.password('');
      await loginPage.username('');
      await loginPage.submitLogin();
      await driver.sleep(1000);

      let result = await loginPage.getToastMessage();
      expect(result).to.equal('Please enter username and password');

      await loginPage.closeToast();
    });

    it('when successful login show logout button, and show form on logout', async () => {
      await loginPage.password('pwd');
      await loginPage.username('user');
      await loginPage.submitLogin();
      await loginPage.logout();

      const form = await loginPage.loginForm();
      const style = await form.getAttribute('style');
      expect(style).to.equal('');
    });

    it('hides menu, hides login and shows message when UI is disabled', async () => {
      let result = await loginPage.getToastMessage();
      expect(result).to.equal('The UI is disabled in the configuration file. See README.');
      await loginPage.closeToast();

      result = await loginPage.menuList();
      const style = await result.getAttribute('style');
      expect(style).to.equal('display: none;');

      const fields = await loginPage.loginFields();
      for (let i = 0; i < fields.length; i++) {
        const field = fields[i];
        const style = await field.getAttribute('style');
        expect(style).to.equal('display: none;');
      }

      const sid = await loginPage.getCookie('SID');
      expect(sid).to.be.null
    });

    it('when session expires reloads the page and lets user login again.', async () => {
      await driver.sleep(1000); // allow fields to load

      const fields = await loginPage.loginFields();
      for (let i = 0; i < fields.length; i++) {
        const field = fields[i];
        const style = await field.getAttribute('style');
        expect(style).to.equal('');
      }

      const result = await loginPage.loginButtonIsDisplayed();
      expect(result, 'Login Button should be displayed').to.be.true;
    });
  });
});
