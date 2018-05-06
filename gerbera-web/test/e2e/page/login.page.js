const {By, until} = require('selenium-webdriver');
const fs = require('fs');

module.exports = function (driver) {

  this.username = async (value) => {
    const field = await driver.findElement(By.id('username'));
    if (value) {
      await field.clear();
      await field.sendKeys(value);
    }
    return field;
  };

  this.password = async (value) => {
    const field = await driver.findElement(By.id('password'));
    if (value) {
      await field.clear();
      await field.sendKeys(value);
    }
    return field;
  };

  this.loginForm = async () => {
    return await driver.findElement(By.id('login-form'));
  };

  this.loginFields = async () => {
    return await driver.findElements(By.css('.login-field'));
  };

  this.loginButtonIsDisplayed = async () => {
    return await driver.findElement(By.id('login-submit')).isDisplayed();
  };

  this.menuList = async () => {
    return await driver.findElement(By.css('ul.navbar-nav'));
  };

  this.logout = async () => {
    const logoutBtn = driver.findElement(By.id('logout'));
    await driver.wait(until.elementIsVisible(logoutBtn), 5000);
    await logoutBtn.click();
    const loginBtn = driver.findElement(By.id('login-submit'));
    return await driver.wait(until.elementIsVisible(loginBtn), 5000);
  };

  this.submitLogin = async () => {
    await driver.findElement(By.id('login-submit')).click();
    return await driver.wait(until.elementLocated(By.id('logout')), 5000);
  };

  this.getToastMessage = async () => {
    await driver.wait(until.elementIsVisible(driver.findElement(By.id('toast'))), 5000);
    return await driver.findElement(By.css('#grb-toast-msg')).getText();
  };

  this.waitForToastClose = async () => {
    await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 6000);
    return await driver.findElement(By.css('#grb-toast-msg')).isDisplayed();
  };

  this.getCookie = async (cookie) => {
    return await driver.manage().getCookie(cookie);
  };

  this.get = async (url) => {
    await driver.get(url);
    try {
      // when reloading session, errorCheck refreshes via JS here....check for staleness first
      await driver.wait(until.stalenessOf(driver.findElement(By.id('navbarContent'))), 2000);
    } catch (e) {
      // the default `page is ready` check...
      await driver.wait(until.elementIsVisible(driver.findElement(By.id('login-form'))), 5000);
    }
    await driver.executeScript('$(\'body\').toggleClass(\'notransition\');');
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('navbarContent'))), 2000);
  };

  this.takeScreenshot = async (filename) => {
    const data = await driver.takeScreenshot();
    fs.writeFileSync(filename, data, 'base64');
  };
};
