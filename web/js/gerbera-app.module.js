/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-app.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2019 Gerbera Contributors

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
/* global Cookies */
import {Auth} from './gerbera-auth.module.js';
import {Autoscan} from './gerbera-autoscan.module.js';
import {Items} from './gerbera-items.module.js';
import {Menu} from './gerbera-menu.module.js';
import {Trail} from './gerbera-trail.module.js';
import {Tree} from './gerbera-tree.module.js';
import {Updates} from './gerbera-updates.module.js';

export class App {

  constructor(clientConfig, serverConfig) {
    this.clientConfig = clientConfig;
    this.serverConfig = serverConfig;
    this.loggedIn = false;
    this.currentTreeItem = undefined;
    this.pageInfo = {};
  }

  isTypeDb() {
    return this.getType() === 'db'
  }

  getType() {
    return Cookies.get('TYPE');
  }

  setType(type) {
    Cookies.set('TYPE', type);
  }

  currentPage() {
    return this.pageInfo.currentPage;
  }

  setCurrentPage(page) {
    this.pageInfo.currentPage = page;
  }

  isLoggedIn(){
    return this.loggedIn;
  }

  setLoggedIn(isLoggedIn){
    this.loggedIn = isLoggedIn;
  }

  initialize () {
    return Updates.initialize()
      .then(() => this.configureDefaults())
      .then(() => {
        return this.getConfig(this.clientConfig);
      })
      .then((response) => {
        return this.loadConfig(response)
      })
      .then((serverConfig) => {
        this.serverConfig = serverConfig;
        return Auth.checkSID();
      })
      .then((loggedIn) => {
        this.loggedIn = loggedIn;
        this.displayLogin(loggedIn);
        Menu.initialize(this.serverConfig);
      })
      .catch((error) => {
        this.error(error);
      });
  }

  configureDefaults () {
    if (this.getType() === undefined) {
      this.setType('db');
    }
    $.ajaxSetup({
      beforeSend: function (xhr) {
        xhr.setRequestHeader('Cache-Control', 'no-cache, must-revalidate');
      }
    });
    return Promise.resolve();
  }

  getConfig (clientConfig) {
    return $.ajax({
      url: clientConfig.api,
      type: 'get',
      async: false,
      data: {
        req_type: 'auth',
        sid: Auth.getSessionId(),
        action: 'get_config'
      }
    });
  }

  loadConfig (response) {
    return new Promise((resolve, reject) => {
      let serverConfig;
      if (response.success) {
        serverConfig = $.extend({}, response.config);
        var pollingInterval = response.config['poll-interval'];
        serverConfig['poll-interval'] = parseInt(pollingInterval) * 1000;
        if(serverConfig.friendlyName && serverConfig.friendlyName !== "Gerbera") {
          $(document).attr('title', serverConfig.friendlyName  + " | Gerbera Media Server");
        }
        resolve(serverConfig);
      } else {
        reject(response);
      }
    });
  }

  viewItems () {
    let viewItemsCount;
    const itemsPerPage = this.serverConfig['items-per-page'];
    if (itemsPerPage && itemsPerPage.default) {
      viewItemsCount = itemsPerPage.default
    } else {
      viewItemsCount = 25
    }
    return viewItemsCount;
  }

  displayLogin (loggedIn) {
    if (loggedIn) {
      $('.login-field').hide();
      $('#login-submit').hide();
      if (this.accounts) {
        $('#logout').show().click(Auth.logout)
      }
      Items.initialize();
      Tree.initialize();
      Trail.initialize();
      Autoscan.initialize();
      Updates.initialize();
    } else {
      $('.login-field').show();
      $('#login-form').submit(function (event) {
        Auth.authenticate(event);
        event.preventDefault();
      });
      $('#logout').hide();
    }
    return Promise.resolve();
  }

  error (event) {
    let message, type, icon;
    if (typeof event === 'string') {
      message = event;
      type = 'warning';
      icon = 'fa-exclamation-triangle';
    } else if (event && event.responseText) {
      message = event.responseText;
      type = 'info';
      icon = 'fa-exclamation-triangle';
    } else if (event && event.error) {
      message = event.error.text;
      type = 'warning';
      icon = 'fa-exclamation-triangle';
    } else {
      message = 'The system encountered an error';
      type = 'danger';
      icon = 'fa-frown-o';
    }
    Updates.showMessage(message, undefined, type, icon)
  }

  disable() {
    Menu.disable();
    Menu.hideLogin();
    Menu.hideMenu();
    Tree.destroy();
    Trail.destroy();
    Items.destroy();
  }

  reload(path) {
    window.location = path;
  }
}

const defaultClientConfig = {
  api: 'content/interface'
};

const defaultServerConfig = {};

export let GerberaApp = new App(defaultClientConfig, defaultServerConfig);