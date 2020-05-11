const {expect} = require('chai');
const TestUtils = require('./page/test-utils');
let driver;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Clients Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('clients.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera clients', () => {
    it('loads the clients when menu is clicked', async () => {
      await homePage.clickMenu('nav-clients');
      const result = await homePage.clients();
      const values = [
        { col: 'ip', head: 'IP Address', index: 1, entry: '192.168.1.12'},
        { col: 'ip', head: 'IP Address', index: 2, entry: '192.168.1.60'},
        { col: 'name', head: 'Profile', index: 1, entry: 'Manual Setup for IP 192.168.1.12'},
        { col: 'name', head: 'Profile', index: 2, entry: 'Standard UPnP'},
        { col: 'userAgent', head: 'User Agent', index: 1, entry: 'UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000'},
        { col: 'userAgent', head: 'User Agent', index: 2, entry: 'UPnP/1.0 DLNADOC/1.50'},
      ];

      expect(result.length).to.equal(3); // head counts also

      for (let index = 0; index < values.length; index++) {
        const v = values[index];
        const client = await homePage.getClientColumn(0, v.col);
        const head = await client.getText();
        expect(head).to.equal(v.head);

        const client2 = await homePage.getClientColumn(v.index, v.col);
        const text = await client2.getText();
        expect(text).to.equal(v.entry);
      };
    });
  });
});