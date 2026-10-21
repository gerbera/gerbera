/*GRB*

    Gerbera - https://gerbera.io/

    test-utils.js - this file is part of Gerbera.

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

const { Builder } = require('selenium-webdriver');
let chrome = require('selenium-webdriver/chrome');
const fs = require('fs');
const path = require('path');
let activeDriver;

const mockWebServer = 'http://' + process.env.npm_package_config_webserver_host + ':' + process.env.npm_package_config_webserver_port;

const newChromeDriver = async () => {
  const chromeOptions = new chrome.Options();
  chromeOptions.addArguments(['--headless', '--window-size=1280,1024', '--incognito']);
  if (process.env.CHROME_BIN) chromeOptions.setChromeBinaryPath(process.env.CHROME_BIN);
  const builder = new Builder()
    .forBrowser('chrome')
    .setChromeOptions(chromeOptions);
  if (process.env.CHROMEDRIVER_BIN) builder.setChromeService(new chrome.ServiceBuilder(process.env.CHROMEDRIVER_BIN));
  activeDriver = await builder.build();
  return activeDriver;
};

const captureFailure = async (label, driver = activeDriver, error) => {
  if (!driver) return;
  const directory = path.resolve(__dirname, '../../../_build/e2e-failures');
  const name = Date.now() + '-' + label.replace(/[^a-z0-9-]/gi, '-').slice(0, 100);
  try {
    fs.mkdirSync(directory, { recursive: true });
    const state = await driver.executeScript(() => ({
      url: location.href,
      title: document.title,
      pendingRequests: window.jQuery && window.jQuery.active,
      fonts: [...document.fonts].map(font => ({ family: font.family, status: font.status })),
      modals: [...document.querySelectorAll('.modal')].map(modal => ({
        id: modal.id, classes: modal.className, display: getComputedStyle(modal).display,
        shown: window.jQuery(modal).data('modal')?._isShown,
        transitioning: window.jQuery(modal).data('modal')?._isTransitioning
      })),
      backdrops: document.querySelectorAll('.modal-backdrop').length
    }));
    state.error = error && { name: error.name, message: error.message, stack: error.stack };
    fs.writeFileSync(path.join(directory, name + '.json'), JSON.stringify(state, null, 2));
    fs.writeFileSync(path.join(directory, name + '.html'), await driver.getPageSource());
    fs.writeFileSync(path.join(directory, name + '.png'), await driver.takeScreenshot(), 'base64');
  } catch (captureError) {
    console.warn('Could not capture browser failure:', captureError.message);
  }
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

const waitForRequests = async driver => driver.wait(() => driver.executeAsyncScript(done => {
  // jQuery schedules its response handlers after resolving the AJAX request.
  setTimeout(() => done(window.jQuery.active === 0), 0);
}), 5000, 'Waiting for pending requests and their response handlers');

module.exports = {
  home,
  newChromeDriver,
  resetCookies,
  resetSuite,
  waitForRequests,
  captureFailure,
  mochaHooks: {
    afterEach: async function () {
      if (this.currentTest.state === 'failed') {
        await captureFailure(this.currentTest.fullTitle(), activeDriver, this.currentTest.err);
      }
    }
  }
};
