/*GRB*

    Gerbera - https://gerbera.io/

    screenshot.doc.js - this file is part of Gerbera.

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

/* global process */
const { Builder } = require('selenium-webdriver');
const { suite } = require('selenium-webdriver/testing');
const yargs = require('yargs');
const { hideBin } = require('yargs/helpers');
const { Jimp } = require('jimp');
let chrome = require('selenium-webdriver/chrome');

const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;

let driver;
let view;

const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');
const argv = yargs(hideBin(process.argv)).parse();

suite(() => {
  let loginPage, homePage, suffix;
  const DEFAULT_FOLDER_STORE = process.cwd() + argv.folderPath;
  console.log('Save path --> ' + DEFAULT_FOLDER_STORE);
  const scheme = argv.scheme;
  view = argv.view;
  before(async () => {
    const chromeOptions = new chrome.Options();
    suffix = '.png';
    chromeOptions.addArguments(['--incognito']);
    chromeOptions.addArguments(['--window-size=1720,1440']);
    if (scheme === 'dark') {
      chromeOptions.addArguments(['--force-dark-mode']);
      suffix = '_dark.png';
    }

    driver = new Builder()
      .forBrowser('chrome')
      .setChromeOptions(chromeOptions)
      .build();
    await driver.get(mockWebServer + '/reset?testName=default.json');
    loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);
  });

  describe('The screenshot documentation spec takes screenshots', () => {

    beforeEach(async () => {
      await driver.get(mockWebServer + '/disabled.html');
      await driver.manage().deleteAllCookies();
      await driver.executeScript("localStorage.clear()");
      await loginPage.get(mockWebServer + '/index.html');
    });

    after(() => driver && driver.quit());

    describe('main-view', () => {
      const viewName = 'main-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [main view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await loginPage.statsVisible();

          await loginPage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w: 1280 });
          image.crop({ x:0, y: 0, w: 1280, h:680 });
          image.write(fileName);
        });
    });

    describe('main-view-empty', () => {
      const viewName = 'main-view-empty';
      if (!view || viewName.match(new RegExp(view)))
        it('for [main view] with empty database', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await loginPage.emptyVisible();

          await loginPage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w: 1280, h: 750 });
          image.write(fileName);
        });
    });

    describe('login-field-entry', () => {
      const viewName = 'login-field-entry';
      if (!view || viewName.match(new RegExp(view)))
        it('for [login] field entry', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('gerbera');
          await loginPage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w: 1440 });
          image.crop({ x:420, y:100, w:600, h:470 });
          image.write(fileName);
        });
    });

    describe('menubar', () => {
      const viewName = 'menubar';
      if (!view || viewName.match(new RegExp(view)))
        it('for [menu bar]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();

          await loginPage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:0, y:0, w:1430, h:80 });
          image.write(fileName);
        });
    });

    describe('database-view', () => {
      const viewName = 'database-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [database view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w:1280, h:700 });
          image.write(fileName);
        });
    });

    describe('database-smallgrid-view', () => {
      const viewName = 'database-smallgrid-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [database small grid view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await homePage.setSelectValue('gridSelect', 1);
          await homePage.setSelectValue('ippSelect', 0);
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w:1280, h:450 });
          image.write(fileName);
        });
    });

    describe('database-largegrid-view', () => {
      const viewName = 'database-largegrid-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [database large grid view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await homePage.setSelectValue('gridSelect', 2);
          await homePage.setSelectValue('ippSelect', 10);
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w:1280, h:500 });
          image.write(fileName);
        });
    });

    describe('database-single-view', () => {
      const viewName = 'database-single-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [database single view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Photos');
          await homePage.setSelectValue('gridSelect', 3);
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.write(fileName);
        });
    });

    describe('search-view', () => {
      const viewName = 'search-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [search view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-search');
          await homePage.clickTree('Photos');
          await homePage.setTextValue('searchQuery', 'upnp:class derivedfrom "object.item.audioItem.musicTack" and dc:title contains "wallaby"');
          await homePage.setTextValue('searchSort', 'upnp:date');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w:1280, h:500 });
          image.write(fileName);
        });
    });

    describe('filesystem-view', () => {
      const viewName = 'filesystem-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [filesystem view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-fs');
          await homePage.clickTree('etc');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.write(fileName);
        });
    });

    describe('clients-view', () => {
      const viewName = 'clients-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [clients view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-clients');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w:1280, h:500 });
          image.write(fileName);
        });
    });

    describe('config-view', () => {
      const viewName = 'config-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [config view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-config');
          await driver.sleep(500);
          await homePage.showConfig('Server (/server)');

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.crop({ x:0, y:0, w:1280, h:750 });
          image.write(fileName);
        });
    });

    describe('items-view', () => {
      const viewName = 'items-view';
      if (!view || viewName.match(new RegExp(view)))
        it('for [items view]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1280 });
          image.write(fileName);
        });
    });

    describe('edit-item', () => {
      const viewName = 'edit-item';
      if (!view || viewName.match(new RegExp(view)))
        it('for [edit item] from item list', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await homePage.editItem(0);
          await driver.sleep(2000);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:500, y:90, w:450, h:875 });
          image.write(fileName);
        });
    });

    describe('edit-item-details', () => {
      const viewName = 'edit-item-details';
      if (!view || viewName.match(new RegExp(view)))
        it('for [edit item] from item list with details', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await homePage.editItem(0);
          await driver.sleep(2000);
          await homePage.showDetails();

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:235, y:75, w:975, h:920 });
          image.write(fileName);
        });
    });

    describe('trail-operations', () => {
      const viewName = 'trail-operations';
      if (!view || viewName.match(new RegExp(view)))
        it('for [trail operations]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:800, y:0, w:630, h:140 });
          image.write(fileName);
        });
    });

    describe('trail-fs-operations', () => {
      const viewName = 'trail-fs-operations';
      if (!view || viewName.match(new RegExp(view)))
        it('for [trail filesystem operations]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-fs');
          await homePage.clickTree('etc');
          await driver.sleep(500);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:860, y:0, w:570, h:140 });
          image.write(fileName);
        });
    });

    describe('edit-autoscan', () => {
      const viewName = 'edit-autoscan';
      if (!view || viewName.match(new RegExp(view)))
        it('for [edit autoscan] from filesystem list', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-fs');
          await homePage.clickTree('etc');
          await homePage.clickTrail(0);
          await driver.sleep(1000);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:380, y:220, w:680, h:660 });
          image.write(fileName);
        });
    });

    describe('edit-autoscan-details', () => {
      const viewName = 'edit-autoscan-details';
      if (!view || viewName.match(new RegExp(view)))
        it('for [edit autoscan] from filesystem list with details', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-fs');
          await homePage.clickTree('etc');
          await homePage.clickTrail(0);
          await driver.sleep(1000);
          await homePage.showAutoscanDetails();

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:235, y:180, w:965, h:740 });
          image.write(fileName);
        });
    });

    describe('edit-tweak-details', () => {
      const viewName = 'edit-tweak-details';
      if (!view || viewName.match(new RegExp(view)))
        it('for [edit tweak] from filesystem list', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-fs');
          await homePage.clickTree('etc');
          await homePage.clickTrail(1);
          await driver.sleep(1000);

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:290, y:0, w:860, h:500 });
          image.write(fileName);
        });
    });

    describe('trail-config-operations', () => {
      const viewName = 'trail-config-operations';
      if (!view || viewName.match(new RegExp(view)))
        it('for [trail config operations]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-config');

          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:860, y:0, w:570, h:140 });
          image.write(fileName);
        });
    });

    describe('item-operations', () => {
      const viewName = 'item-operations';
      if (!view || viewName.match(new RegExp(view)))
        it('for [item edit operations]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');

          await homePage.getItem(0);
          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:325, y:125, w:1115, h:125 });
          image.write(fileName);
        });
    });

    describe('toast-message', () => {
      const viewName = 'toast-message';
      if (!view || viewName.match(new RegExp(view)))
        it('for [toast message]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-db');
          await homePage.clickTree('Video');
          await homePage.clickTree('All Video');

          await homePage.getToastElement();
          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:0, y:990, w:1440, h:100 });
          image.write(fileName);
        });
    });

    describe('task-message', () => {
      const viewName = 'task-message';
      if (!view || viewName.match(new RegExp(view)))
        it('for [task message]', async () => {
          const fileName = DEFAULT_FOLDER_STORE + viewName + suffix;
          await loginPage.username('user');
          await loginPage.password('pwd');
          await loginPage.submitLogin();
          await homePage.clickMenu('nav-fs');
          await homePage.clickTree('etc');

          await homePage.mockTaskMessage('Scan of /movies');
          await driver.sleep(1000);
          await homePage.takeScreenshot(fileName);

          const image = await Jimp.read(fileName);
          image.resize({ w:1440 });
          image.crop({ x:0, y:990, w:1440, h:100 });
          image.write(fileName);
        });
    });
  });
});
