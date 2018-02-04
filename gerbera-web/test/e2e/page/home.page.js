var webdriver = require('selenium-webdriver')
var By = webdriver.By
var until = webdriver.until

module.exports = function (driver) {
  this.getDatabaseMenu = function () {
    return driver.findElement(By.id('nav-db'))
  }
  this.getFileSystemMenu = function () {
    return driver.findElement(By.id('nav-fs'))
  }
  this.clickMenu = function (menuId) {
    var tree = driver.findElement(By.id('tree'))
    if (menuId === 'nav-home') {
      return driver.findElement(By.id(menuId)).click()
    } else {
      driver.findElement(By.id(menuId)).click()
      return driver.wait(until.elementIsVisible(tree), 5000)
    }
  }
  this.isDisabled = function (id) {
    return driver.wait(until.elementLocated(By.css('#' + id + '.disabled')), 5000).then(function (el) {
      return true
    })
  }
  this.treeItems = function () {
    return driver.findElements(By.css('#tree li'))
  }
  this.treeItemChildItems = function (text) {
    return driver.findElements(By.xpath('//li[.//span[contains(text(),\'' + text + '\')]]')).then(function (items) {
      // the xpath finds both <database> and <Video>  TODO: needs work
      return items[1].findElements(By.css('li.list-group-item'))
    })
  }
  this.clickTree = function (text) {
    var elem = driver.findElement(By.xpath('//span[contains(text(),\'' + text + '\')]'))
    driver.wait(until.elementIsVisible(elem), 5000)
    elem.click()
    return driver.sleep(500) // todo use wait...
  }
  this.expandTree = function (text) {
    return driver.findElements(By.xpath('//li[.//span[contains(text(),\'' + text + '\')]]')).then(function (items) {
      var folderTitle = items[1].findElement(By.className('folder-title'))
      folderTitle.click()
      return driver.sleep(500) // todo use wait...
    })
  }

  this.getItem = function (idx) {
    driver.wait(until.elementLocated(By.id('datagrid'), 5000))
    return driver.findElements(By.className('grb-item')).then(function (items) {
      return items[idx]
    })
  }

  this.deleteItemFromList = function (idx) {
    return driver.findElements(By.css('.grb-item span.grb-item-delete')).then(function (items) {
      var item = items[idx]
      item.click()
      return driver.wait(until.stalenessOf(item))
    })
  }

  this.editItem = function (idx) {
    return driver.findElements(By.css('.grb-item span.grb-item-edit')).then(function (items) {
      items[idx].click()
      return driver.wait(until.elementIsVisible(driver.findElement(By.id('editModal'))), 5000)
    })
  }

  this.editOverlayDisplayed = function () {
    driver.sleep(1000) // allow for animation
    return driver.findElement(By.id('editModal')).isDisplayed()
  }

  this.editOverlayFieldValue = function (fieldName) {
    return driver.findElement(By.id(fieldName)).then(function (element) {
      return element.getAttribute('value')
    })
  }

  this.editorOverlayField = function (fieldName) {
    return driver.findElement(By.id(fieldName))
  }

  this.setEditorOverlayField = function (fieldName, value) {
    return this.editorOverlayField(fieldName).then(function (field) {
      field.clear()
      return field.sendKeys(value)
    })
  }

  this.editOverlaySubmitText = function () {
    return driver.findElement(By.id('editSave')).getText()
  }

  this.editOverlayParentId = function () {
    return driver.findElement(By.id('addParentIdTxt')).getText()
  }

  this.autoscanOverlayDisplayed = function () {
    driver.sleep(1000) // await animation
    return driver.findElement(By.id('autoscanModal')).isDisplayed()
  }

  this.cancelEdit = function () {
    driver.sleep(200) // slow down with animations
    driver.findElement(By.id('editCancel')).click()
    driver.sleep(1000)
    return driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000)
  }

  this.submitEditor = function () {
    driver.findElement(By.id('editSave')).click()
    return driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000)
  }

  this.items = function () {
    driver.wait(until.elementLocated(By.id('datagrid')), 1000)
    return driver.findElements(By.className('grb-item'))
  }

  this.hasDeleteIcon = function (grbItem) {
    return grbItem.findElement(By.css('.grb-item-delete')).isDisplayed()
  }

  this.hasEditIcon = function (grbItem) {
    return grbItem.findElement(By.css('.grb-item-edit')).isDisplayed()
  }

  this.hasDownloadIcon = function (grbItem) {
    return grbItem.findElement(By.css('.grb-item-download')).isDisplayed()
  }

  this.hasAddIcon = function (grbItem) {
    return grbItem.findElement(By.css('.grb-item-add')).isDisplayed()
  }

  this.hasTrailAddIcon = function () {
    return driver.findElement(By.css('.grb-trail-add')).isDisplayed().then(function (displayed) {
      return displayed
    }, function () {
      return false
    })
  }

  this.hasTrailAddAutoscanIcon = function () {
    return driver.findElement(By.css('.grb-trail-add-autoscan')).isDisplayed()
  }

  this.hasTrailDeleteIcon = function () {
    return driver.wait(until.elementLocated(By.css('.grb-trail-delete')), 1000).then(function (elem) {
      return elem.isDisplayed()
    }, function () {
      return false
    })
  }

  this.clickTrailAdd = function () {
    return driver.findElement(By.css('.grb-trail-add')).then(function (addEl) {
      addEl.click()
      return driver.wait(until.elementIsVisible(driver.findElement(By.id('editModal'))), 5000)
    })
  }

  this.clickTrailAddAutoscan = function () {
    return driver.findElement(By.css('.grb-trail-add-autoscan')).then(function (addEl) {
      addEl.click()
      return driver.wait(until.elementIsVisible(driver.findElement(By.id('autoscanModal'))), 5000)
    })
  }

  this.clickTrailEdit = function () {
    return driver.findElement(By.css('.grb-trail-edit')).then(function (el) {
      el.click()
      return driver.wait(until.elementIsVisible(driver.findElement(By.id('editModal'))), 5000)
    })
  }

  this.editOverlayTitle = function () {
    driver.wait(until.elementIsVisible(driver.findElement(By.css('.modal-title'))), 5000)
    return driver.findElement(By.css('.modal-title')).getText()
  }

  this.selectObjectType = function (objectType) {
    return driver.findElement(By.css('#editObjectType>option[value=\'' + objectType + '\']')).click()
  }

  this.getAutoscanModeTimed = function () {
    return driver.findElement(By.id('autoscanModeTimed')).getAttribute('checked')
  }

  this.getAutoscanLevelBasic = function () {
    return driver.findElement(By.id('autoscanLevelBasic')).getAttribute('checked')
  }

  this.getAutoscanRecursive = function () {
    return driver.findElement(By.id('autoscanRecursive')).getAttribute('checked')
  }

  this.getAutoscanHidden = function () {
    return driver.findElement(By.id('autoscanHidden')).getAttribute('checked')
  }

  this.getAutoscanIntervalValue = function () {
    return driver.findElement(By.id('autoscanInterval')).getAttribute('value')
  }

  this.cancelAutoscan = function () {
    driver.findElement(By.id('autoscanCancel')).click()
    driver.sleep(1000)
    return driver.wait(until.elementIsNotVisible(driver.findElement(By.id('autoscanModal'))), 5000)
  }

  this.submitAutoscan = function () {
    driver.findElement(By.id('autoscanSave')).click()
    return driver.wait(until.elementIsNotVisible(driver.findElement(By.id('autoscanModal'))), 5000)
  }

  this.clickAutoscanEdit = function (idx) {
    return driver.findElements(By.css('.autoscan')).then(function (items) {
      return items[idx].click()
    })
  }

  this.getToastMessage = function () {
    return driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000).then(function () {
      return driver.findElement(By.css('.grb-toast-msg')).getText()
    })
  }

  this.getToastElement = function () {
    return driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000).then(function () {
      return driver.findElement(By.id('toast'))
    })
  }

  this.waitForToastClose = function () {
    driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 6000)
    return driver.findElement(By.css('.grb-toast-msg')).isDisplayed()
  }

  this.getPages = function () {
    return driver.findElements(By.css('.page-item'))
  }

  this.getWindowSize = function () {
    return driver.manage().window().getSize();
  }
}
