const {Builder} = require('selenium-webdriver');
let chrome = require('selenium-webdriver/chrome');
const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;

const newChromeDriver = async () => {
  const chromeOptions = new chrome.Options();
  chromeOptions.addArguments(['--headless', '--window-size=1280,1024']);
  return new Builder()
    .forBrowser('chrome')
    .setChromeOptions(chromeOptions)
    .build();
};

const resetSuite = async (expectations, driver) => {
  return driver.get(mockWebServer + '/reset?testName=' + expectations);
};

const resetCookies = async (driver) => {
  await driver.get(mockWebServer + '/disabled.html');
  return driver.manage().deleteAllCookies();
};

const home = () => {
  return mockWebServer + '/index.html';
};

module.exports = {
  home,
  newChromeDriver,
  resetCookies,
  resetSuite,
};