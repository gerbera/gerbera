/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-app.module.js - this file is part of Gerbera.

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
import { Auth } from './gerbera-auth.module.js';
import { Autoscan } from './gerbera-autoscan.module.js';
import { Items } from './gerbera-items.module.js';
import { Menu } from './gerbera-menu.module.js';
import { Trail } from './gerbera-trail.module.js';
import { Tweaks } from "./gerbera-tweak.module.js";
import { Tree } from './gerbera-tree.module.js';
import { Updates } from './gerbera-updates.module.js';
import { Clients } from './gerbera-clients.module.js';
import { Config } from './gerbera-config.module.js';

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
      search: '',
      sort: '',
      currentPage: 0,
      viewItems: -1,
      viewItemsOld: -1,
      gridMode: 0,
      displayMode: 'default',
      currentItem: {
        'home': [],
        'db': [],
        'search': [],
        'fs': [],
        'clients': [],
        'config': [],
      }
    };
    this.navLinks = {
      'home': '#nav-home',
      'db': '#nav-db',
      'search': '#nav-search',
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

  getSearch() {
    return [this.pageInfo.search, this.pageInfo.sort];
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

  setSearch(query, sort) {
    this.pageInfo.search = query;
    this.pageInfo.sort = sort;
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

  setGridMode(newValue) {
    if (newValue === 3) {
      if (this.pageInfo.viewItems !== 1)
        this.pageInfo.viewItemsOld = this.pageInfo.viewItems;
      this.pageInfo.viewItems = 1;
    } else if (this.pageInfo.gridMode === 3) {
      if (this.pageInfo.viewItemsOld)
        this.pageInfo.viewItems = this.pageInfo.viewItemsOld;
      else
        this.pageInfo.viewItems = this.serverConfig['items-per-page'] ? this.serverConfig['items-per-page'].default : -1;
    }

    this.pageInfo.gridMode = newValue;
    this.writeLocalStorage();
    return this.pageInfo.viewItems;
  }

  setViewItems(items) {
    if (this.pageInfo.viewItems !== items)
      this.pageInfo.viewItemsOld = this.pageInfo.viewItems;
    this.pageInfo.viewItems = items;
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

  isLoggedIn() {
    return this.loggedIn;
  }

  setLoggedIn(isLoggedIn) {
    this.loggedIn = isLoggedIn;
    if (this.loggedIn) {
      this.getStatus(this.clientConfig)
        .then((response) => { return this.displayStatus(response); })
        .catch((error) => { this.error(error); });
    }
  }

  initialize() {
    $('#server-status').hide();

    this.pageInfo = {
      dbType: 'home',
      configMode: 'minimal',
      currentPage: 0,
      viewItems: -1,
      gridMode: 0,
      currentItem: {
        'home': [],
        'db': [],
        'search': [],
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
        }
        if (!('configMode' in this.pageInfo)) {
          this.pageInfo.configMode = 'minimal';
        }
        if (!('gridMode' in this.pageInfo)) {
          this.pageInfo.gridMode = 0;
        }
        if (!('displayMode' in this.pageInfo)) {
          this.pageInfo.displayMode = 'light dark';
        }
        this.loadDisplayMode(this.pageInfo.displayMode);

        if (!('viewItems' in this.pageInfo)) {
          const itemsPerPage = this.serverConfig['items-per-page'];
          if (itemsPerPage && itemsPerPage.default) {
            this.pageInfo.viewItems = itemsPerPage.default;
          }
        }
        if (this.pageInfo.dbType && this.pageInfo.dbType in this.navLinks) {
          $(this.navLinks[this.pageInfo.dbType]).click();
        }
        this.initDone = true;
      })
      .catch((error) => {
        this.error(error);
      });
  }

  configureDefaults() {
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

  getStatus(clientConfig) {
    var data = {
      req_type: 'config_load',
      action: 'status',
    };
    data[Auth.SID] = Auth.getSessionId();
    return $.ajax({
      url: clientConfig.api,
      type: 'get',
      data: data,
      async: false,
      accepts: 'application/json',
      headers: {
        Accept: "application/json; charset=utf-8",
        "Content-Type": "application/json; charset=utf-8"
      },
    });
  }

  getStatusValue(itemList, entry) {
    const found = itemList.filter(i => i.item === '/status/attribute::' + entry);
    if (found && found.length > 0) {
      return found[0].value;
    }
    return "";
  }

  showStatus(itemList, key) {
    const cnt = this.getStatusValue(itemList, key + 'Count');
    if (cnt && cnt !== '') {
      $('#status-' + key + '-count').html(cnt);
      $('#status-' + key + '-size').html(this.getStatusValue(itemList, key + 'Size'));
      return Number.parseInt(cnt);
    } else {
      $('#status-' + key).hide();
      return 0;
    }
  }

  displayStatus(response) {
    let cnt = 0;
    if (response.success && response.values) {
      cnt += this.showStatus(response.values.item, 'total');
      cnt += this.showStatus(response.values.item, 'audio');
      cnt += this.showStatus(response.values.item, 'audioBook');
      cnt += this.showStatus(response.values.item, 'audioMusic');
      cnt += this.showStatus(response.values.item, 'audioBroadcast');
      cnt += this.showStatus(response.values.item, 'video');
      cnt += this.showStatus(response.values.item, 'videoMovie');
      cnt += this.showStatus(response.values.item, 'videoBroadcast');
      cnt += this.showStatus(response.values.item, 'videoMusicVideoClip');
      cnt += this.showStatus(response.values.item, 'image');
      cnt += this.showStatus(response.values.item, 'imagePhoto');
      cnt += this.showStatus(response.values.item, 'text');
      cnt += this.showStatus(response.values.item, 'item');
    }
    if (cnt == 0) {
      $('#server-empty').show();
    } else {
      $('#server-status').show();
    }
    return Promise.resolve();
  }

  getConfig(clientConfig) {
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

  loadConfig(response) {
    return new Promise((resolve, reject) => {
      let serverConfig;
      if (response.success) {
        serverConfig = $.extend({}, response.config);
        var pollingInterval = response.config['poll-interval'];
        serverConfig['poll-interval'] = parseInt(pollingInterval) * 1000;
        if (serverConfig.friendlyName && serverConfig.friendlyName !== "Gerbera") {
          $(document).attr('title', serverConfig.friendlyName + " | Gerbera Media Server");
        }
        resolve(serverConfig);
      } else {
        reject(response);
      }
    });
  }

  loadDisplayMode(mode) {
    document.documentElement.style.setProperty('color-scheme', mode);
    if (mode == 'light dark')
      mode = 'default';
    document.getElementById('displayMode_' + mode).checked = true;
  }

  setDisplayMode(event) {
    let mode = event.target.value;
    if (mode == 'default')
      mode = 'light dark';
    document.documentElement.style.setProperty('color-scheme', mode);
    this.pageInfo.displayMode = mode;
    this.writeLocalStorage();
  }

  viewItems() {
    return this.pageInfo.viewItems >= 0 ? this.pageInfo.viewItems : (this.serverConfig['items-per-page'] ? this.serverConfig['items-per-page'].default : 25);
  }

  gridMode() {
    return this.pageInfo.gridMode;
  }

  itemsPerPage() {
    if (this.serverConfig['items-per-page'] && this.serverConfig['items-per-page'].option && this.serverConfig['items-per-page'].default)
      return this.serverConfig['items-per-page'];
    return { option: [10, 25, 50, 100], default: 25 };
  }

  displayLogin(loggedIn) {
    $('#server-status').hide();
    $('#server-empty').hide();
    $('#homelinks').hide();
    if (loggedIn) {
      $('#login').hide();
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
      this.getStatus(this.clientConfig)
        .then((response) => { return this.displayStatus(response); })
        .catch((error) => { this.error(error); });
    } else {
      $('#login').show();
      $('#login-form').submit(function (event) {
        Auth.authenticate(event);
        event.preventDefault();
      });
      $('#logout').hide();
    }
    return Promise.resolve();
  }

  error(event) {
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
  api: 'interface'
};

const defaultServerConfig = {};

export let GerberaApp = new App(defaultClientConfig, defaultServerConfig);
