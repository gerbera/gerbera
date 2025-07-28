/*GRB*

    Gerbera - https://gerbera.io/

    config.spec.js - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

      let result = await homePage.editOverlayFieldValue('grb_value__server_modelNumber');
      expect(result).to.equal('2.6.0');

      await homePage.setEditorOverlayField('grb_value__server_modelNumber', '43');

      result = await homePage.editOverlayFieldValue('grb_value__server_modelNumber');
      expect(result).to.equal('43');

      await homePage.clickTrail(1);

      result = await homePage.getToastMessage();
      expect(result).to.equal('Successfully saved 1 config items');
    });
  });
});

