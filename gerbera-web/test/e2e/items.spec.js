/*GRB*

    Gerbera - https://gerbera.io/

    items.spec.js - this file is part of Gerbera.

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

describe('Items Suite', () => {
  let loginPage, homePage;

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('items.spec.json', driver);

    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);

    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  describe('The gerbera items', () => {
    it('loads the related items when tree item is clicked', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const result = await homePage.items();
      expect(result.length).to.equal(12);
    });

    it('a gerbera item contains the edit icon', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const item = await homePage.getItem(2);
      const result = await homePage.hasEditIcon(item);
      expect(result).to.be.true;
    });

    it('a gerbera item contains the download icon', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const item = await homePage.getItem(2);
      const result = await homePage.hasDownloadIcon(item);
      expect(result).to.be.true;
    });

    it('a gerbera item contains the delete icon', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Video');
      const item = await homePage.getItem(2);
      const result = await homePage.hasDeleteIcon(item);
      expect(result).to.be.true;
    });

    it('loads child tree items when tree item is expanded', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.expandTree('Video');
      const items = await homePage.treeItemChildItems('Video');
      expect(items.length).to.equal(2);
    });

    it('a gerbera file item does not contain any modify icons', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');

      const item = await homePage.getItem(2);

      let result = await homePage.hasEditIcon(item);
      expect(result).to.be.false;

      result = await homePage.hasDeleteIcon(item);
      expect(result).to.be.false;

      result = await homePage.hasDownloadIcon(item);
      expect(result).to.be.false;
    });

    it('a gerbera file item contains no add icon', async () => {
      await homePage.clickMenu('nav-fs');
      await homePage.clickTree('etc');
      const item = await homePage.getItem(1);
      const result = await homePage.hasAddIcon(item);
      expect(result).to.be.false;
    });

    it('the gerbera items contains a pager that retrieves pages of items', async () => {
      await homePage.clickMenu('nav-db');
      await homePage.clickTree('Playlists');

      const pages = await homePage.getPages();
      expect(pages).to.have.lengthOf(4);

      const items = await homePage.items();
      expect(items).to.have.lengthOf(26);
      await pages[2].click();

      const item = await homePage.getItem(0);
      const text = await item.getText();
      expect(text).to.equal('Test25.mp4\n(1 MB, 1:42:00.448, 720x756)');
    });
  });
});
