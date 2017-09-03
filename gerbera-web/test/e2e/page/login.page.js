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

  this.logout = function () {
    var logoutBtn = driver.findElement(By.id('logout'))
    driver.wait(until.elementIsVisible(logoutBtn), 5000)
    return logoutBtn.click()
  }

  this.submitLogin = function () {
    return driver.findElement(By.id('login-submit')).click()
  }

  this.warning = function () {
    return driver.findElement(By.id('warning')).getText()
  }

  this.get = function (url) {
    return driver.get(url)
  }
}
