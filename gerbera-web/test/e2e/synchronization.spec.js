/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera. If not, see <http://www.gnu.org/licenses/>.
*/
const assert = require('node:assert/strict');
const { By, until } = require('selenium-webdriver');
const TestUtils = require('./page/test-utils');
const HomePage = require('./page/home.page');
const LoginPage = require('./page/login.page');

describe('Browser synchronization', () => {
  let driver, homePage;

  const delayRequest = async type => driver.executeScript(type => {
    const open = XMLHttpRequest.prototype.open;
    const send = XMLHttpRequest.prototype.send;
    window.delayedResponseComplete = false;
    XMLHttpRequest.prototype.open = function (method, url, ...args) {
      this.delayed = String(url).includes('req_type=' + type);
      return open.call(this, method, url, ...args);
    };
    XMLHttpRequest.prototype.send = function (...args) {
      if (this.delayed) {
        this.addEventListener('loadend', () => { window.delayedResponseComplete = true; });
        setTimeout(() => send.apply(this, args), 1200);
      } else send.apply(this, args);
    };
    window.restoreRequest = () => {
      XMLHttpRequest.prototype.open = open;
      XMLHttpRequest.prototype.send = send;
    };
  }, type);

  before(async () => {
    driver = await TestUtils.newChromeDriver();
    await TestUtils.resetSuite('overlay.test.json', driver);
    const loginPage = new LoginPage(driver);
    homePage = new HomePage(driver);
    await TestUtils.resetCookies(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
  });

  after(() => driver && driver.quit());

  beforeEach(async () => {
    await homePage.clickMenu('nav-db');
    await homePage.clickTree('Video');
  });

  afterEach(async () => {
    await homePage.cancelEdit();
  });

  it('does not try to close an editor that never opened', async () => {
    await homePage.cancelEdit();
    assert.equal(await driver.findElement(By.id('editModal')).isDisplayed(), false);
  });

  it('does not try to close an editor that was already dismissed', async () => {
    await homePage.editItem(0);
    await driver.findElement(By.css('#editModal .close')).click();
    await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000);
    await homePage.cancelEdit();
  });

  it('waits for an editor control that becomes interactable later', async () => {
    await driver.executeScript(() => {
      const icon = document.querySelector('#datagrid .grb-item-edit');
      icon.style.visibility = 'hidden';
      setTimeout(() => { icon.style.visibility = ''; }, 1200);
    });
    await homePage.editItem(0);
    assert.equal(await homePage.editOverlayFieldValue('editTitle'), 'My Title');
  });

  it('reacquires a control replaced while the view is loading', async () => {
    await driver.executeScript(() => {
      const icon = document.querySelector('#datagrid .grb-item-edit');
      icon.style.visibility = 'hidden';
      setTimeout(() => {
        const replacement = window.jQuery(icon).clone(true, true)[0];
        icon.replaceWith(replacement);
        replacement.style.visibility = '';
      }, 1200);
    });
    await homePage.editItem(0);
    assert.equal(await homePage.editOverlayFieldValue('editTitle'), 'My Title');
  });

  it('waits for opening animation before dismissing the editor', async () => {
    await driver.executeScript(() => {
      document.querySelector('#editModal .modal-dialog').style.transitionDuration = '1.2s';
    });
    try {
      await driver.findElement(By.css('#datagrid .grb-item-edit')).click();
      await homePage.cancelEdit();
      assert.equal((await driver.findElements(By.css('.modal-backdrop'))).length, 0);
    } finally {
      await driver.executeScript(() => {
        document.querySelector('#editModal .modal-dialog').style.transitionDuration = '';
      });
    }
  });

  it('waits for replacement tree data when changing menu', async () => {
    await delayRequest('directories');
    try {
      await homePage.clickMenu('nav-fs');
      assert.equal((await homePage.treeItems()).length, 19);
    } finally {
      await driver.executeScript(() => window.restoreRequest());
    }
  });

  it('waits for item responses instead of reading the previous grid', async () => {
    await delayRequest('items');
    try {
      await homePage.clickTree('Video');
      assert.equal(await driver.executeScript(() => window.delayedResponseComplete), true);
      assert.equal((await homePage.items()).length, 12);
    } finally {
      await driver.executeScript(() => window.restoreRequest());
    }
  });

  it('still fails when the requested editor control does not exist', async () => {
    await assert.rejects(homePage.editItem(999), { name: 'TimeoutError' });
  });

  it('waits for the selected page instead of returning items from the old page', async () => {
    await TestUtils.resetSuite('items.spec.json', driver);
    const loginPage = new LoginPage(driver);
    await loginPage.get(TestUtils.home());
    await loginPage.password('pwd');
    await loginPage.username('user');
    await loginPage.submitLogin();
    await homePage.clickMenu('nav-db');
    await homePage.clickTree('Playlists');
    await delayRequest('items');
    try {
      const pages = await homePage.getPages();
      await pages[2].click();
      const item = await homePage.getItem(0);
      assert.equal(await driver.executeScript(() => window.delayedResponseComplete), true);
      assert.equal(await item.getText(), 'Test25.mp4\n(1 MB, 1:42:00.448, 720x756)');
    } finally {
      await driver.executeScript(() => window.restoreRequest());
    }
  });
});
