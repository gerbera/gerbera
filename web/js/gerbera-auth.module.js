/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-auth.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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
/* global md5 */
import {GerberaApp} from './gerbera-app.module.js';
import {Tree} from './gerbera-tree.module.js';
import {Items} from './gerbera-items.module.js';
import {Menu} from './gerbera-menu.module.js';
import {Trail} from './gerbera-trail.module.js';
import {Autoscan} from './gerbera-autoscan.module.js';
import {Updates} from './gerbera-updates.module.js';

const SID = 'GerberaSID';

const checkSID = () => {
  var data = {
    req_type: 'auth',
    action: 'get_sid'
  }
  data[SID] = getSessionId();
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: data,
    async: false,
  }).then((response) => {
    return loadSession(response);
  }).catch((err) => {
    GerberaApp.error(err);
  });
};

const logout = () => {
  var data = {
    req_type: 'auth',
    action: 'logout'
  };
  data[SID] = getSessionId();
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: data,
    async: false,
  }).then(() => {
    handleLogout();
  }).catch((err) => {
    GerberaApp.error(err);
  });
};

const authenticate = () => {
  const username = $('#username').val();
  const password = $('#password').val();
  let promise;

  if (username.length > 0 && password.length > 0) {
    $('#warning').hide();
    var data = {
      req_type: 'auth',
      action: 'get_token'
    }
    data[SID] = getSessionId();
    promise =
      $.ajax({
        url: GerberaApp.clientConfig.api,
        data: data,
        type: 'get',
      }).then((response) => {
        return submitLogin(response);
      }).catch((err) => {
        GerberaApp.error(err);
      });
  } else {
    Updates.showMessage('Please enter username and password', undefined, 'warning', 'fa-sign-in');
    promise = Promise.resolve({});
  }
  return promise
};

const submitLogin = (response) => {
  if (response.success) {
    const username = $('#username').val();
    let password = $('#password').val();
    const token = response.token;
    password = md5(token + password);
    var data = {
      req_type: 'auth',
      action: 'login',
      username: username,
      password: password
    }
    data[SID] = getSessionId();
    return $.ajax({
      url: GerberaApp.clientConfig.api,
      data: data,
      type: 'get',
    }).then((response) => {
      checkLogin(response);
    }).catch((err) => {
      GerberaApp.error(err);
    });
  } else {
    Updates.showMessage(response.error.text, undefined, 'warning', 'fa-exclamation-circle')
  }
};

const checkLogin = (response) => {
  if (response.success) {
    $('#login').hide();
    if (GerberaApp.serverConfig.accounts) {
      $('#logout').show().click(logout);
    }
    $('#homelinks').show();
    $('#navbar li').show();
    GerberaApp.setLoggedIn(true);
    Tree.initialize();
    Items.initialize();
    Trail.initialize();
    Menu.initialize();
    Autoscan.initialize();
    Updates.initialize();
  }
};

const getSessionId = () => {
  return Cookies.get(SID);
};

const handleLogout = () => {
  GerberaApp.setLoggedIn(false);
  const now = new Date();
  const expire = new Date();
  expire.setTime(now.getTime() - 3600000 * 24 * 360);
  Cookies.set(SID, null, {expires: expire, SameSite: 'Lax'});
  GerberaApp.reload('/index.html');
};

const loadSession = (response) => {
  return new Promise((resolve) => {
    if ((!response.sid_was_valid || !getSessionId()) && response[SID] && response[SID] !== null) {
      console.log(response);
      Cookies.set(SID, response[SID], {SameSite: 'Lax'});
    }
    // TODO: reject what?
    resolve(response.logged_in);
  });
};

export const Auth = {
  SID,
  authenticate,
  checkSID,
  getSessionId,
  handleLogout,
  logout,
};
