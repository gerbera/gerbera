/*GRB*

    Gerbera - https://gerbera.io/

    test-utils.js - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

const { Builder } = require('selenium-webdriver');
let chrome = require('selenium-webdriver/chrome');

const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;

const newChromeDriver = async () => {
  const chromeOptions = new chrome.Options();
  chromeOptions.addArguments(['--headless', '--window-size=1280,1024', '--incognito']);
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
  await driver.executeScript("localStorage.clear()");
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