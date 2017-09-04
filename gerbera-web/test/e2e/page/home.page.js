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
    return driver.findElement(By.id(menuId)).click()
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
    return driver.findElement(By.xpath('//span[contains(text(),\'' + text + '\')]')).click()
  }
  this.expandTree = function (text) {
    return driver.findElements(By.xpath('//li[.//span[contains(text(),\'' + text + '\')]]')).then(function (items) {
      var folderTitle = items[1].findElement(By.className('folder-title'))
        folderTitle.click()
      return driver.sleep(500) // todo use wait...
    })
  }
  this.items = function () {
    return driver.findElements(By.className('grb-item'))
  }
}
