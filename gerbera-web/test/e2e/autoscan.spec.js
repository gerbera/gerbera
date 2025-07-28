/*GRB*

    Gerbera - https://gerbera.io/

    autoscan.spec.js - this file is part of Gerbera.

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

describe('Autoscan Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('autoscan.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);

    await loginPage.get(TestUtils.home());

    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The autoscan overlay', () => {

    it('a gerbera tree fs item container opens autoscan overlay when trail add autoscan is clicked', async () => {
      await homePage.clickMenu('nav-fs');
      driver.sleep(500); // allow for load
      await homePage.clickTree('etc');
      await homePage.clickTrailAddAutoscan();

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.setAutoscanMode('Timed');
      result = await homePage.getAutoscanMode('Timed');
      expect(result).to.equal('true');

      result = await homePage.getAutoscanRecursive();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanHidden();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanIntervalValue();
      expect(result).to.equal('1800');

      await homePage.cancelAutoscan();
    });

    it('a gerbera tree fs item container opens autoscan overlay when trail add autoscan is clicked and inotify is activated', async () => {
      await homePage.clickMenu('nav-fs');
      driver.sleep(500); // allow for load
      await homePage.clickTree('etc');
      await homePage.clickTrailAddAutoscan();

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.setAutoscanMode('Inotify');
      result = await homePage.getAutoscanMode('Inotify');
      expect(result).to.equal('true');
      result = await homePage.getAutoscanMode('Timed');
      expect(result).to.equal(null);

      result = await homePage.getAutoscanRecursive();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanHidden();
      expect(result).to.equal('true');

      result = await homePage.getAutoscanFollowSymlink();
      expect(result).to.equal('true');

      await homePage.cancelAutoscan();
    });

    it('autoscan displays message when properly submitted to server', async () => {
      await homePage.clickMenu('nav-fs');
      driver.sleep(500); // allow for load
      await homePage.clickTree('etc');
      await homePage.clickTrailAddAutoscan();

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;

      await homePage.showAutoscanDetails();

      result = await homePage.getAutoscanId();
      expect(result).to.equal('2f6d');
      await homePage.setAutoscanMode('Inotify');
      await homePage.setAutoscanRecursive();
      await homePage.submitAutoscan();

      result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.false;

      result = await homePage.getToastMessage();
      expect(result).to.equal('Scan: /Movies');

      await homePage.closeToast();
    });

    it('an existing autoscan item loads edit overlay', async () => {
      await homePage.clickMenu('nav-db');
      driver.sleep(500); // allow for load
      await homePage.clickTree('Playlists');
      await homePage.clickAutoscanEdit(1);

      let result = await homePage.autoscanOverlayDisplayed();
      expect(result).to.be.true;
    });
  });
});

