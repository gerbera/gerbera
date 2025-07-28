/*GRB*

    Gerbera - https://gerbera.io/

    trail.spec.js - this file is part of Gerbera.

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

describe('Trail Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('trail.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera trail', () => {

    it('a gerbera tree item shows no add icon in the trail and no delete item', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      let result = await homePage.hasTrailAddIcon();
      expect(result).to.be.false;

      result = await homePage.hasTrailAddAutoscanIcon();
      expect(result).to.be.true;

      result = await homePage.hasTrailDeleteIcon();
      expect(result).to.be.false;
    });

    it('a gerbera tree db item container shows delete icon and add icon in the trail', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');

      let result = await homePage.hasTrailDeleteIcon();
      expect(result).to.be.true;

      result = await homePage.hasTrailAddIcon();
      expect(result).to.be.true;
    });
  });
});

