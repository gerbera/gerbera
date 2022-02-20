/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-app.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2022 Gerbera Contributors

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
import {Auth} from './gerbera-auth.module.js';
import {Autoscan} from './gerbera-autoscan.module.js';
import {Items} from './gerbera-items.module.js';
import {Menu} from './gerbera-menu.module.js';
import {Trail} from './gerbera-trail.module.js';
import {Tweaks} from "./gerbera-tweak.module.js";
import {Tree} from './gerbera-tree.module.js';
import {Updates} from './gerbera-updates.module.js';
import {Clients} from './gerbera-clients.module.js';
import {Config} from './gerbera-config.module.js';

export class App {
  constructor(clientConfig, serverConfig) {
    this.clientConfig = clientConfig;
    this.serverConfig = serverConfig;
    this.loggedIn = false;
    this.currentTreeItem = undefined;
    this.initDone = false;
    this.pageInfo = {
      dbType: 'home',
      configMode: 'minimal',
      currentPage: 0,
      viewItems: 25,
      gridMode: 0,
      currentItem: {
        'home': [],
        'db': [],
        'fs': [],
        'clients': [],
        'config': [],
      }
    };
    this.navLinks = {
      'home': '#nav-home',
      'db': '#nav-db',
      'fs': '#nav-fs',
      'clients': '#nav-clients',
      'config': '#nav-config',
    };
  }

  isTypeDb() {
    return this.getType() === 'db'
  }

  getType() {
    return this.pageInfo.dbType;
  }

  writeLocalStorage() {
    if (this.initDone) {
      localStorage.setItem('pageInfo', JSON.stringify(this.pageInfo));
    }
  }

  setType(type) {
    this.pageInfo.dbType = type;
    this.writeLocalStorage();
  }

  currentItem() {
    if (this.pageInfo.currentItem && this.pageInfo.dbType in this.pageInfo.currentItem) {
      return this.pageInfo.currentItem[this.pageInfo.dbType];
    }
    return [];
  }

  currentPage() {
    return this.pageInfo.currentPage;
  }

  configMode() {
    return this.pageInfo.configMode;
  }

  setCurrentPage(page) {
    this.pageInfo.currentPage = page;
    this.writeLocalStorage();
  }

  setCurrentConfig(mode) {
    this.pageInfo.configMode = mode;
    this.writeLocalStorage();
  }

  setGridMode(mode) {
    this.pageInfo.gridMode = mode;
    this.writeLocalStorage();
  }

  setViewItems(mode) {
    this.pageInfo.viewItems = mode;
    this.writeLocalStorage();
  }

  setCurrentItem(item) {
    var parentElement = item.target;
    var tree = [];
    var cnt = 0;
    while (parentElement && cnt < 100) {
      cnt++; // avoid inifinite loop
      if (parentElement.id && parentElement.id !== '') {
        tree.unshift(parentElement.id);
      }
      parentElement = parentElement.parentElement;
    }
    this.pageInfo.currentItem[this.pageInfo.dbType] = tree;
    this.writeLocalStorage();
  }

  isLoggedIn(){
    return this.loggedIn;
  }

  setLoggedIn(isLoggedIn){
    this.loggedIn = isLoggedIn;
    if (this.loggedIn) {
        this.getStatus(this.clientConfig).then((response) => { return this.displayStatus(response); });
    }
  }

  initialize () {
    $('#server-status').hide();

    this.pageInfo = {
      dbType: 'home',
      configMode: 'minimal',
      currentPage: 0,
      viewItems: 25,
      gridMode: 0,
      currentItem: {
        'home': [],
        'db': [],
        'fs': [],
        'clients': [],
        'config': [],
      }
    };
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
      .then(() => {
        if (localStorage.getItem('pageInfo')) {
          this.pageInfo = JSON.parse(localStorage.getItem('pageInfo'));
          if (!('configMode' in this.pageInfo)) {
            this.pageInfo.configMode = 'minimal';
          }
          if (!('gridMode' in this.pageInfo)) {
            this.pageInfo.gridMode = 0;
          }
          if (!('viewItems' in this.pageInfo)) {
            const itemsPerPage = this.serverConfig['items-per-page'];
            if (itemsPerPage && itemsPerPage.default) {
              this.pageInfo.viewItems = itemsPerPage.default;
            } else {
              this.pageInfo.viewItems = 25;
          }}
          if(this.pageInfo.dbType && this.pageInfo.dbType in this.navLinks) {
            $(this.navLinks[this.pageInfo.dbType]).click();
          }
        }
        this.initDone = true;
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

  getStatus (clientConfig) {
    var data = {
      req_type: 'config_load',
      action: 'status'
    };
    data[Auth.SID] = Auth.getSessionId();
    return $.ajax({
      url: clientConfig.api,
      type: 'get',
      data: data,
      async: false,
    });
  }

  getStatusValue(itemList, entry) {
    const found = itemList.filter(i => i.item === '/status/attribute::' + entry);
    if (found && found.length > 0) {
      return found[0].value;
    }
    return "";
  }

  displayStatus (response) {
    if (response.success && response.values) {
      $('#server-status').show();
      $('#status-total').html(this.getStatusValue(response.values.item, 'total'));
      $('#status-audio').html(this.getStatusValue(response.values.item, 'audio'));
      $('#status-video').html(this.getStatusValue(response.values.item, 'video'));
      $('#status-image').html(this.getStatusValue(response.values.item, 'image'));
    }
    return Promise.resolve();
  }

  getConfig (clientConfig) {
    var data = {
      req_type: 'auth',
      action: 'get_config'
    };
    data[Auth.SID] = Auth.getSessionId();
    return $.ajax({
      url: clientConfig.api,
      type: 'get',
      data: data,
      async: false,
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
    return this.pageInfo.viewItems;
  }

  gridMode () {
    return this.pageInfo.gridMode;
  }

  itemsPerPage () {
    if (this.serverConfig['items-per-page'] && this.serverConfig['items-per-page'].option)
      return this.serverConfig['items-per-page'].option;
    return [10,25,50,100];
  }

  displayLogin (loggedIn) {
    if (loggedIn) {
      $('.login-field').hide();
      $('#login-submit').hide();
      if (this.serverConfig.accounts) {
        $('#logout').show().click(Auth.logout);
      }
      $('#homelinks').show();
      $('#navbar li').show();
      Items.initialize();
      Tree.initialize();
      Trail.initialize();
      Autoscan.initialize();
      Updates.initialize();
      Clients.initialize();
      Config.initialize();
      Tweaks.initialize();
      this.getStatus(this.clientConfig).then((response) => {return this.displayStatus(response); });
    } else {
      $('#server-status').hide();
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
    Updates.showMessage(message, undefined, type, icon);
  }

  disable() {
    Menu.disable();
    Menu.hideLogin();
    Menu.hideMenu();
    Tree.destroy();
    Trail.destroy();
    Items.destroy();
    Clients.destroy();
    Config.destroy();
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
