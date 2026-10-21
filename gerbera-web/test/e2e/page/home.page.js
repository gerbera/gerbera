/*GRB*

    Gerbera - https://gerbera.io/

    home.page.js - this file is part of Gerbera.

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

const { By, until, error } = require('selenium-webdriver');
const fs = require('fs');
const TestUtils = require('./test-utils');

module.exports = function (driver) {
  const requestsComplete = () => TestUtils.waitForRequests(driver);

  const clickWhenReady = async (locator, index = 0) => {
    return driver.wait(async () => {
      const elements = await driver.findElements(locator);
      const element = elements[index];
      if (!element) return false;
      try {
        if (!await element.isDisplayed() || !await element.isEnabled()) return false;
        await element.click();
        return element;
      } catch (err) {
        if (err instanceof error.StaleElementReferenceError ||
            err instanceof error.ElementNotInteractableError ||
            err instanceof error.ElementClickInterceptedError) return false;
        throw err;
      }
    }, 5000, 'Waiting to click ' + locator);
  };

  const clickMenu = async (menuId, locator) => {
    const previous = await driver.findElements(By.css('#tree li'));
    await clickWhenReady(locator);
    if (menuId === 'nav-db' || menuId === 'nav-fs' || menuId === 'nav-search') {
      if (previous.length) await driver.wait(until.stalenessOf(previous[0]), 5000);
      await driver.wait(until.elementLocated(By.css('#tree li')), 5000);
      await driver.wait(until.elementIsVisible(driver.findElement(By.id('tree'))), 5000);
    }
  };

  const editorReady = async () => driver.wait(() => driver.executeScript(() => {
    const modal = window.jQuery('#editModal').data('modal');
    return !!modal && modal._isShown && !modal._isTransitioning;
  }), 5000, 'Waiting for the editor opening transition');

  const editorClosed = async () => driver.wait(async () => {
    const modal = await driver.findElement(By.id('editModal'));
    return await modal.getCssValue('display') === 'none' &&
      (await driver.findElements(By.css('.modal-backdrop'))).length === 0;
  }, 5000, 'Waiting for the editor and its backdrop to close');

  this.getDatabaseMenu = async () => {
    return await driver.findElement(By.id('nav-db'));
  };

  this.getFileSystemMenu = async () => {
    return await driver.findElement(By.id('nav-fs'));
  };

  this.getSearchMenu = async () => {
    return await driver.findElement(By.id('nav-search'));
  };

  this.getClientsMenu = async () => {
    return await driver.findElement(By.id('nav-clients'));
  };

  this.getConfigMenu = async () => {
    return await driver.findElement(By.id('nav-config'));
  };

  this.getHomeMenu = async () => {
    return await driver.findElement(By.id('nav-home'));
  };

  this.getTitle = async () => {
    return await driver.getTitle();
  };

  this.clickMenu = async (menuId) => {
    return clickMenu(menuId, By.id(menuId));
  };

  this.clickMenuIcon = async (menuId) => {
    return clickMenu(menuId, By.css('#' + menuId + ' i'));
  };
  this.isDisabled = async (id) => {
    try {
      await driver.wait(until.elementLocated(By.css('#' + id + '.disabled')), 5000);
    } catch (e) {
      console.log(e);
      return false;
    }
    return true;
  };
  this.treeItems = async () => {
    return await driver.findElements(By.css('#tree li'));
  };
  this.treeItemChildItems = async (text) => {
    const items = await driver.findElements(By.xpath('//li[.//span[contains(text(),\'' + text + '\')]]'));
    // the xpath finds both <database> and <Video>  TODO: needs work
    return await items[1].findElements(By.css('li.list-group-item'));
  };
  this.showConfig = async (text) => {
    const elem = await driver.findElement(By.xpath('//span[contains(text(),\'' + text + '\')]'));
    await driver.wait(until.elementIsVisible(elem), 5000);
    await elem.click();
    return await driver.sleep(500); // todo use wait...
  };
  this.clickTree = async (text) => {
    const elem = await driver.findElement(By.xpath('//span[contains(text(),\'' + text + '\')]'));
    await driver.wait(until.elementIsVisible(elem), 5000);
    await elem.click();
    return requestsComplete();
  };
  this.clickTrail = async (index) => {
    const elem = await driver.findElements(By.css('.grb-trail-button'));
    await elem[index].click();
    return await driver.sleep(500)
  };
  this.expandTree = async (text) => {
    const items = await driver.findElements(By.xpath('//li[.//span[contains(text(),\'' + text + '\')]]'));
    const folderTitle = await items[0].findElement(By.className('folder-title'));
    await folderTitle.click();
    return await driver.sleep(500); // todo use wait...
  };

  this.getItem = async (idx) => {
    await requestsComplete();
    await driver.wait(until.elementLocated(By.css('#datagrid .grb-item')), 5000);
    const items = await driver.findElements(By.css('#datagrid .grb-item'));
    return items[idx];
  };

  this.deleteItemFromList = async (idx) => {
    const item = await clickWhenReady(By.css('#datagrid .grb-item span.grb-item-delete'), idx);
    return await driver.wait(until.stalenessOf(item), 5000);
  };

  this.editItem = async (idx) => {
    await clickWhenReady(By.css('#datagrid .grb-item span.grb-item-edit'), idx);
    return editorReady();
  };

  this.editOverlayDisplayed = async () => {
    await editorReady();
    return await driver.findElement(By.id('editModal')).isDisplayed();
  };

  this.editOverlayFieldValue = async (fieldName) => {
    const el = await driver.findElement(By.id(fieldName));
    return await el.getAttribute('value');
  };

  this.editOverlayFieldAttribute = async (fieldName, attName) => {
    const el = await driver.findElement(By.id(fieldName));
    return await el.getAttribute(attName);
  };

  this.editorOverlayField = async (fieldName) => {
    return await driver.findElement(By.id(fieldName));
  };

  this.editorOverlayFieldDisplayed = async (fieldName) => {
    return await driver.findElement(By.id(fieldName)).isDisplayed();
  };

  this.setEditorOverlayField = async (fieldName, value) => {
    const field = await this.editorOverlayField(fieldName);
    await field.clear();
    return await field.sendKeys(value);
  };

  this.editOverlaySubmitText = async () => {
    return await driver.findElement(By.id('editSave')).getText();
  };

  this.showDetails = async () => {
    return await driver.findElement(By.id('detailbutton')).click();
  };

  this.tweaksOverlayDisplayed = async () => {
    await driver.sleep(1000); // await animation
    return await driver.findElement(By.id('dirTweakModal')).isDisplayed();
  };

  this.cancelEdit = async () => {
    try {
      // A failed test can leave an edit-load request in flight.
      await requestsComplete();
      const opening = await driver.executeScript(() => {
        const modal = window.jQuery('#editModal').data('modal');
        return !!modal && modal._isShown;
      });
      if (opening) {
        await editorReady();
        await clickWhenReady(By.css('#editModal #editCancel'));
      }
      return await editorClosed();
    } catch (err) {
      await TestUtils.captureFailure('editor-cleanup', driver, err);
      throw err;
    }
  };

  this.submitEditor = async () => {
    await editorReady();
    await clickWhenReady(By.id('editSave'));
    return editorClosed();
  };

  this.items = async () => {
    await requestsComplete();
    await driver.wait(until.elementLocated(By.id('datagrid')), 1000);
    return await driver.findElements(By.css('#datagrid .grb-item'));
  };

  this.hasDeleteIcon = async (grbItem) => {
    try {
      return await grbItem.findElement(By.css('.grb-item-delete')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.hasEditIcon = async (grbItem) => {
    try {
      return await grbItem.findElement(By.css('.grb-item-edit')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.hasDownloadIcon = async (grbItem) => {
    try {
      return await grbItem.findElement(By.css('.grb-item-download')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.hasAddIcon = async (grbItem) => {
    try {
      await driver.wait(until.elementLocated(By.css('.grb-trail-add')), 500);
      return grbItem.findElement(By.css('.grb-item-add')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.hasTrailAddIcon = async () => {
    try {
      await driver.wait(until.elementLocated(By.css('.grb-trail-add')), 500);
      return await driver.findElement(By.css('.grb-trail-add')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.hasTrailAddAutoscanIcon = async () => {
    return await driver.findElement(By.css('.grb-trail-add-autoscan')).isDisplayed();
  };

  this.hasTrailDeleteIcon = async () => {
    try {
      await driver.wait(until.elementLocated(By.css('.grb-trail-delete')), 1000);
      return driver.findElement(By.css('.grb-trail-delete')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.clickTrailAdd = async () => {
    await clickWhenReady(By.css('.grb-trail-add'));
    return editorReady();
  };

  this.clickTrailAddAutoscan = async () => {
    const el = await driver.findElement(By.css('.grb-trail-add-autoscan'));
    await el.click();
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('autoscanModal'))), 5000);
  };

  this.clickTrailTweak = async () => {
    const el = await driver.findElement(By.css('.grb-trail-tweak-dir'));
    await el.click();
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('dirTweakModal'))), 5000);
  };

  this.clickTrailEdit = async () => {
    await clickWhenReady(By.css('.grb-trail-edit'));
    return editorReady();
  };

  this.editOverlayTitle = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.css('.modal-title'))), 5000);
    return await driver.findElement(By.css('.modal-title')).getText();
  };

  this.selectObjectType = async (objectType) => {
    return await driver.findElement(By.css('#editObjectType>option[value=\'' + objectType + '\']')).click();
  };

  this.showAutoscanDetails = async () => {
    return await driver.findElement(By.id('detailAutoscanButton')).click();
  };

  this.setAutoscanMode = async (mode) => {
    return await driver.findElement(By.id('autoscanMode' + mode)).click();
  };

  this.autoscanOverlayDisplayed = async () => {
    await driver.sleep(1000); // await animation
    return await driver.findElement(By.id('autoscanModal')).isDisplayed();
  };

  this.getAutoscanMode = async (mode) => {
    return await driver.findElement(By.id('autoscanMode' + mode)).getAttribute('checked');
  };

  this.getAutoscanRecursive = async () => {
    return await driver.findElement(By.id('autoscanRecursive')).getAttribute('checked');
  };

  this.getAutoscanId = async () => {
    return await driver.findElement(By.id('autoscanId')).getAttribute('value');
  };

  this.setAutoscanRecursive = async () => {
    return await driver.findElement(By.id('autoscanRecursive')).click();
  };

  this.getAutoscanHidden = async () => {
    return await driver.findElement(By.id('autoscanHidden')).getAttribute('checked');
  };

  this.getAutoscanFollowSymlink = async () => {
    return await driver.findElement(By.id('autoscanSymlinks')).getAttribute('checked');
  };

  this.getAutoscanIntervalValue = async () => {
    return await driver.findElement(By.id('autoscanInterval')).getAttribute('value');
  };

  this.cancelAutoscan = async () => {
    await driver.findElement(By.id('autoscanCancel')).click();
    await driver.sleep(1000);
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('autoscanModal'))), 5000);
  };

  this.cancelTweaks = async () => {
    await driver.findElement(By.id('dirTweakCancel')).click();
    await driver.sleep(1000);
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('dirTweakModal'))), 5000);
  };

  this.submitAutoscan = async () => {
    await driver.findElement(By.id('autoscanSave')).click();
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('autoscanModal'))), 5000);
  };

  this.submitTweaks = async () => {
    await driver.findElement(By.id('dirTweakSave')).click();
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('dirTweakModal'))), 5000);
  };

  this.clickAutoscanEdit = async (idx) => {
    const items = await driver.findElements(By.css('.autoscan'));
    return await items[idx].click();
  };

  this.getToastMessage = async () => {
    let toastMsg = await driver.findElement(By.id('grb-toast-msg'));
    await driver.wait(until.elementIsVisible(toastMsg), 5000);
    return await toastMsg.getText();
  };

  this.getToastElement = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000);
    return await driver.findElement(By.id('toast'));
  };

  this.getToastElementWidth = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000);
    return await driver.executeScript('return $(\'#toast\').width()');
  };

  this.closeToast = async () => {
    await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000);
    await driver.findElement(By.css('#toast button.close')).click();
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 2000);
  };

  this.getPages = async () => {
    return await driver.findElements(By.css('.page-item'));
  };

  this.setSelectValue = async (key, value) => {
    var dropDown = await driver.findElement(By.id(key));
    await dropDown.click();
    //Select value from first dropDown
    await dropDown.findElement(By.css("option[value='" + value + "']")).click();
    await driver.sleep(500); // wait a bit
  };

  this.setTextValue = async (key, value) => {
    var field = await driver.findElement(By.id(key));
    if (value && field) {
      await field.clear();
      await field.sendKeys(value);
    }
    return field;
  };

  this.clients = async () => {
    await driver.wait(until.elementLocated(By.css('#clientgrid.with-data')), 5000);
    return await driver.findElements(By.className('grb-client'));
  };

  this.getClientColumn = async (idx, col) => {
    await driver.wait(until.elementLocated(By.id('clientgrid')), 1000);
    const clients = await driver.findElements(By.className('grb-client-' + col));
    return clients[idx];
  };

  this.takeScreenshot = async (filename) => {
    const data = await driver.takeScreenshot();
    fs.writeFileSync(filename, data, 'base64');
  };

  this.mockTaskMessage = async (msg) => {
    return await driver.executeScript('return $(\'#toast\').toast(\'showTask\', {message: "' + msg + '", type: "info", icon: "fa-refresh fa-spin fa-fw"});')
  };

  this.getVersion = async () => {
    return await driver.findElement(By.css('#gerbera-version span'));
  };
};
