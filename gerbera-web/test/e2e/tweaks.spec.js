/*GRB*

    Gerbera - https://gerbera.io/

    tweaks.spec.js - this file is part of Gerbera.

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
      expect(result).to.equal('Successfully saved 1 config items');

      await homePage.closeToast();
    });
  });
});

