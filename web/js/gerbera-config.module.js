/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-config.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

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
import {GerberaApp} from './gerbera-app.module.js';
import {Auth} from './gerbera-auth.module.js';
import {Trail} from './gerbera-trail.module.js';
import {Updates} from "./gerbera-updates.module.js";

const destroy = () => {
  const datagrid = $('#configgrid');
  if (datagrid.hasClass('grb-config')) {
    datagrid.config('destroy');
  } else {
    datagrid.html('');
  }
};

let current_config = {
  config: null,
  values: null,
  meta: null,
  changedItems: {},
  choice: 'expert',
  chooser: {
    minimal: {caption: 'Minimal', fileName: './gerbera-config-minimal.json'},
    standard: {caption: 'Standard', fileName: './gerbera-config-standard.json'},
    expert: {caption: 'Expert', fileName: './gerbera-config-expert.json'}
  }
};

const initialize = () => {
  current_config.config = null;
  current_config.values = null;
  current_config.meta = null;
  current_config.choice = GerberaApp.configMode();
  $('#configgrid').html('');
  return Promise.resolve();
};

const menuSelected = () => {
  current_config.choice = GerberaApp.configMode();
  retrieveGerberaConfig()
    .then((response) => {
      if (typeof response === 'string') {
        loadConfig(JSON.parse(response), 'config');
      } else {
        loadConfig(response, 'config');
      }
    })
    .catch((err) => GerberaApp.error(err));
  retrieveGerberaValues('config_load')
    .then((response) => {
      loadConfig({success: response.success, values: response.values.item}, 'values');
      if ('types' in response) {
        loadConfig({success: response.success, meta: response.types.item}, 'meta');
      } else {
        loadConfig({success: response.success, meta: []}, 'meta');
      }
    })
    .catch((err) => GerberaApp.error(err));
};

const retrieveGerberaConfig = () => {
  return $.ajax({
    url: current_config.chooser[current_config.choice].fileName
  });
};

const retrieveGerberaValues = (type) => {
  var requestData = {
    req_type: type
  };
  requestData[Auth.SID] = Auth.getSessionId();
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: requestData
  });
};

const configModeChanged = (mode) => {
  current_config.choice = mode;
  const oldValues = current_config.values;
  const oldMeta = current_config.meta;
  GerberaApp.setCurrentConfig(mode);
  initialize();
  retrieveGerberaConfig()
    .then((response) => {
      if (typeof response === 'string') {
        loadConfig(JSON.parse(response), 'config');
      } else {
        loadConfig(response, 'config');
      }
      loadConfig({success: true, values: oldValues}, 'values');
      loadConfig({success: true, meta: oldMeta}, 'meta');
    })
    .catch((err) => GerberaApp.error(err));
};

const loadConfig = (response, item) => {
  if (response.success && item in response && response[item]) {
     current_config[item] = response[item];
  }
  if (current_config.config !== null && current_config.values !== null && current_config.meta !== null) {
    const datagrid = $('#configgrid');

    if (datagrid.hasClass('grb-config')) {
      datagrid.config('destroy');
    }
    current_config.changedItems = {};
    datagrid.config({
      setup: current_config.config,
      values: current_config.values,
      meta: current_config.meta,
      choice: current_config.choice,
      chooser: current_config.chooser,
      addResultItem: setChangedItem,
      configModeChanged: configModeChanged,
      itemType: 'config'
    });
    Trail.makeTrailFromItem({
      parent_id: -1
    });
  }
};

const setChangedItem = (itemValue) => {
  current_config.changedItems[itemValue.item] = itemValue;
};

const saveConfig = () => {
  const changedKeys = Object.getOwnPropertyNames(current_config.changedItems);
  if (changedKeys.length > 0 ) {
    const saveData = {
      req_type: 'config_save',
      data: [],
      changedCount: changedKeys.length,
      updates: 'check'
    };
    saveData[Auth.SID] = Auth.getSessionId();
    changedKeys.forEach((key) => {
      let i = current_config.changedItems[key];
      if (i.item && i.item !== '') {
        saveData.data.push({
          item: i.item,
          id: i.id,
          value: i.value,
          origValue: i.origValue,
          status: i.status});
      }
    });

    try {
      $.ajax({
        url: GerberaApp.clientConfig.api,
        type: 'get',
        data: saveData
      })
        .then(() => {
          menuSelected();
          Updates.showMessage('Successfully saved ' + saveData.data.length + ' config items', undefined, 'success', 'fa-check');
        })
        .catch((err) => GerberaApp.error(err));
    } catch (e) {
      GerberaApp.error(e);
    }
  } else {
    console.log("Nothing to save");
  }
};

const clearConfig = () => {
  const saveData = {
    req_type: 'config_save',
    data: [],
    action: 'clear',
    changedCount: -1,
    updates: 'check'
  };
  saveData[Auth.SID] = Auth.getSessionId();

  try {
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: saveData
    })
      .then(() => {
        menuSelected();
      })
      .catch((err) => GerberaApp.error(err));
  } catch (e) {
    GerberaApp.error(e);
  }
};

const reScanLibrary = () => {
  const saveData = {
    req_type: 'config_save',
    data: [],
    action: 'rescan',
    target: '--all',
    changedCount: -1,
    updates: 'check'
  };
  saveData[Auth.SID] = Auth.getSessionId();

  try {
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: saveData
    })
      .then(() => {
        menuSelected();
      })
      .catch((err) => {
        GerberaApp.error(err)
      });
  } catch (e) {
    GerberaApp.error(e);
  }
};

export const Config = {
  destroy,
  loadConfig,
  saveConfig,
  clearConfig,
  reScanLibrary,
  initialize,
  menuSelected,
};
