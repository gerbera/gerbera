/*GRB*

    Gerbera - https://gerbera.io/

    clients.spec.js - this file is part of Gerbera.

    Copyright (C) 2016-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

const { expect } = require('chai');

const TestUtils = require('./page/test-utils');
const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

let driver;

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
        { col: 'ip', head: 'IP Address\nHost Name\nGroup', index: 1, entry: '192.168.1.12\nMyPC\ndefault' },
        { col: 'ip', head: 'IP Address\nHost Name\nGroup', index: 2, entry: '192.168.1.60\nBluRay\ndefault' },
        { col: 'name', head: 'Profile\nMatch Type\nMatch Pattern', index: 1, entry: 'Manual Setup for IP 192.168.1.12\nIP\n192.168.1.12' },
        { col: 'name', head: 'Profile\nMatch Type\nMatch Pattern', index: 2, entry: 'Standard UPnP\nUserAgent\nUPnP/1.0' },
        { col: 'userAgent', head: 'User Agent', index: 1, entry: 'UPnP/1.0 DLNADOC/1.50 Platinum/1.0.4.2-bb / foobar2000' },
        { col: 'userAgent', head: 'User Agent', index: 2, entry: 'UPnP/1.0 DLNADOC/1.50' },
      ];

      expect(result.length).to.equal(5); // head counts also

      for (let index = 0; index < values.length; index++) {
        const v = values[index];
        const client = await homePage.getClientColumn(0, v.col);
        const head = await client.getText();
        expect(head).to.equal(v.head);

        const client2 = await homePage.getClientColumn(v.index, v.col);
        const text = await client2.getText();
        expect(text).to.equal(v.entry);
      }
    });
  });
});