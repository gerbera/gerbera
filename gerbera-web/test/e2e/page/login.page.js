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
    return await driver.findElement(By.id('login'));
  };

  this.loginButtonIsDisplayed = async () => {
    return await driver.findElement(By.id('login-submit')).isDisplayed();
  };

  this.statsVisible = async () => {
    const stats = await driver.findElement(By.id('server-status'));
    return await driver.wait(until.elementIsVisible(stats), 5000);
  };

  this.emptyVisible = async () => {
    await driver.executeScript('return $(\'#server-empty\').show();')
    await driver.executeScript('return $(\'#server-status\').hide();')
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
    let toastMsg = await driver.findElement(By.id('grb-toast-msg'));
    await driver.wait(until.elementIsVisible(toastMsg), 5000);
    return await toastMsg.getText();
  };

  this.closeToast = async () => {
    await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('editModal'))), 5000);
    await driver.findElement(By.css('#toast button.close')).click();
    return await driver.wait(until.elementIsNotVisible(driver.findElement(By.id('toast'))), 2000);
  };

  this.getCookie = async (cookie) => {
    let result;
    try {
      result = await driver.manage().getCookie(cookie);
    } catch(err) {
      result = null;
    }
    return Promise.resolve(result);
  };

  this.get = async (url) => {
    await driver.get(url);
    try {
      // when reloading session, errorCheck refreshes via JS here....check for staleness first
      await driver.wait(until.stalenessOf(driver.findElement(By.id('navbarContent'))), 3000);
    } catch (e) {
      // the default `page is ready` check...
      await driver.wait(until.elementIsVisible(driver.findElement(By.id('homeintro'))), 5000);
    }
    await driver.executeScript('$(\'body\').toggleClass(\'notransition\');');
    return await driver.wait(until.elementIsVisible(driver.findElement(By.id('navbarContent'))), 2000);
  };

  this.takeScreenshot = async (filename) => {
    const data = await driver.takeScreenshot();
    fs.writeFileSync(filename, data, 'base64');
  };
};
