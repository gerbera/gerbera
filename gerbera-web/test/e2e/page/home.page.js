const {By, until} = require('selenium-webdriver');
const fs = require('fs');

module.exports = function (driver) {
  this.getDatabaseMenu = async () => {
    return await driver.findElement(By.id('nav-db'));
  };

  this.getFileSystemMenu = async () => {
    return await driver.findElement(By.id('nav-fs'));
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
    if (menuId === 'nav-home' || menuId === 'nav-clients' || menuId == 'nav-config') {
      return await driver.findElement(By.id(menuId)).click()
    } else {
      const tree = await driver.findElement(By.id('tree'));
      await driver.findElement(By.id(menuId)).click();
      return await driver.wait(until.elementIsVisible(tree), 5000)
    }
  };

  this.clickMenuIcon = async (menuId) => {
    const tree = await driver.findElement(By.id('tree'));
    if (menuId === 'nav-home' || menuId === 'nav-clients'|| menuId == 'nav-config') {
      return await driver.findElement(By.css('#'+ menuId + ' i')).click();
    } else {
      await driver.findElement(By.css('#'+ menuId + ' i')).click();
      return await driver.wait(until.elementIsVisible(tree), 5000);
    }
  };
  this.isDisabled = async (id) => {
    await driver.wait(until.elementLocated(By.css('#' + id + '.disabled')), 5000);
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
    return await driver.sleep(500); // todo use wait...
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
    await driver.wait(until.elementLocated(By.id('datagrid'), 5000));
    const items = await driver.findElements(By.className('grb-item'));
    return items[idx];
  };

  this.deleteItemFromList = async (idx) => {
    const items = await driver.findElements(By.css('.grb-item span.grb-item-delete'));
    const item = items[idx];
    await item.click();
    return await driver.wait(until.stalenessOf(item));
  };

  this.editItem = async (idx) => {
    const items = await driver.findElements(By.css('.grb-item span.grb-item-edit'));
    await items[idx].click();
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('editModal'))), 5000);
  };

  this.editOverlayDisplayed = async () => {
    await driver.sleep(1000); // allow for animation
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

  this.editorOverlayFieldDisplayed = async(fieldName) => {
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

  this.autoscanOverlayDisplayed = async () => {
    await driver.sleep(1000); // await animation
    return await driver.findElement(By.id('autoscanModal')).isDisplayed();
  };

  this.tweaksOverlayDisplayed = async () => {
    await driver.sleep(1000); // await animation
    return await driver.findElement(By.id('dirTweakModal')).isDisplayed();
  };

  this.cancelEdit = async () => {
    await driver.sleep(500); // slow down with animations
    await driver.findElement(By.id('editCancel')).click();
    await driver.sleep(1000);
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000);
  };

  this.submitEditor = async () => {
    await driver.findElement(By.id('editSave')).click();
    await driver.sleep(500); // todo: wait on .modal-backdrop somehow
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000);
  };

  this.items = async () => {
    await driver.wait(until.elementLocated(By.id('datagrid')), 1000);
    return await driver.findElements(By.className('grb-item'));
  };

  this.hasDeleteIcon = async (grbItem) => {
    try {
      return await grbItem.findElement(By.css('.grb-item-delete')).isDisplayed();
    } catch(e) {
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
      return grbItem.findElement(By.css('.grb-item-add')).isDisplayed();
    } catch (e) {
      return false;
    }
  };

  this.hasTrailAddIcon = async () => {
    return await driver.findElement(By.css('.grb-trail-add')).isDisplayed();
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
    const addEl = await driver.findElement(By.css('.grb-trail-add'));
    await addEl.click();
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('editModal'))), 5000);
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
    const el = await driver.findElement(By.css('.grb-trail-edit'));
    await el.click();
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('editModal'))), 5000);
  };

  this.editOverlayTitle = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.css('.modal-title'))), 5000);
    return await driver.findElement(By.css('.modal-title')).getText();
  };

  this.selectObjectType = async (objectType) => {
    return await driver.findElement(By.css('#editObjectType>option[value=\'' + objectType + '\']')).click();
  };

  this.getAutoscanModeTimed = async () => {
    return await driver.findElement(By.id('autoscanModeTimed')).getAttribute('checked');
  };

  this.getAutoscanRecursive = async () => {
    return await driver.findElement(By.id('autoscanRecursive')).getAttribute('checked');
  };

  this.getAutoscanHidden = async () => {
    return await driver.findElement(By.id('autoscanHidden')).getAttribute('checked');
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
    await driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000);
    return await driver.findElement(By.css('#grb-toast-msg')).getText();
  };

  this.getToastElement = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000);
    return await driver.findElement(By.id('toast'));
  };

  this.getToastElementWidth = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000);
    return await driver.executeScript('return $(\'#toast\').width()');
  };

  this.waitForToastClose = async () => {
    await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 6000);
    return await driver.findElement(By.css('#grb-toast-msg')).isDisplayed();
  };

  this.closeToast = async () => {
    await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000);
    await driver.findElement(By.css('#toast button.close')).click();
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 2000);
  };

  this.getPages = async () => {
    return await driver.findElements(By.css('.page-item'));
  };

  this.clients = async () => {
    await driver.wait(until.elementLocated(By.id('clientgrid')), 1000);
    return await driver.findElements(By.className('grb-client'));
  };

  this.getClientColumn = async (idx, col) => {
    await driver.wait(until.elementLocated(By.id('clientgrid'), 1000));
    const clients = await driver.findElements(By.className('grb-client-' + col));
    return clients[idx];
  };

  this.takeScreenshot = async (filename) => {
    const data = await driver.takeScreenshot();
    fs.writeFileSync(filename, data, 'base64');
  };

  this.mockTaskMessage = async (msg) => {
    return await driver.executeScript('return $(\'#toast\').toast(\'showTask\', {message: "'+ msg +'", type: "info", icon: "fa-refresh fa-spin fa-fw"});')
  };

  this.getVersion = async () => {
    return await driver.findElement(By.css('#gerbera-version span'));
  };
};
