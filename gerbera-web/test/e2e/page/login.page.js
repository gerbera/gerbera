var By = require('selenium-webdriver').By
var until = require('selenium-webdriver').until

module.exports = function (driver) {

  this.username = function (value) {
    var field = driver.findElement(By.id('username'))
    if (value) {
      field.clear()
      field.sendKeys(value)
    }
    return field
  }

  this.password = function (value) {
    var field = driver.findElement(By.id('password'))
    if (value) {
      field.clear()
      field.sendKeys(value)
    }
    return field
  }

  this.loginForm = function () {
    return driver.findElement(By.id('login-form'))
  }

  this.loginFields = function () {
    return driver.findElements(By.css('.login-field'))
  }

  this.menuList = function () {
    return driver.findElement(By.css('ul.navbar-nav'))
  }

  this.logout = function () {
    var logoutBtn = driver.findElement(By.id('logout'))
    driver.wait(until.elementIsVisible(logoutBtn), 5000)
    logoutBtn.click()
    var loginBtn = driver.findElement(By.id('login-submit'))
    return driver.wait(until.elementIsVisible(loginBtn), 5000)
  }

  this.submitLogin = function () {
    driver.findElement(By.id('login-submit')).click()
    return driver.wait(until.elementLocated(By.id('logout')), 5000)
  }

  this.getToastMessage = function () {
    driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000)
    return driver.findElement(By.css('.grb-toast-msg')).getText()
  }

  this.waitForToastClose = function () {
    driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 6000)
    return driver.findElement(By.css('.grb-toast-msg')).isDisplayed()
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

  this.getContentHTML = function () {
    return driver.findElement(By.id('content')).getText()
  }

  this.get = function (url) {
    driver.get(url)
    var loginForm = driver.findElement(By.id('login-form'))
    return driver.wait(until.elementIsEnabled(loginForm), 5000)
  }
}
