/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-config.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2020 Gerbera Contributors

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

let current_config = { config: null, values: null, changedItems: {} };

const initialize = () => {
  $('#configgrid').html('');
  return Promise.resolve();
};

const menuSelected = () => {
  retrieveGerberaConfig()
    .then((response) => {
      loadConfig(JSON.parse(response), 'config');
      //loadConfig(JSON.parse(response), 'values');
    })
    .catch((err) => GerberaApp.error(err));
  retrieveGerberaValues('config_load')
    .then((response) => loadConfig({success: response.success, values: response.values.item}, 'values'))
    .catch((err) => GerberaApp.error(err));
};

const retrieveGerberaConfig = () => {
  return $.ajax({
    url: 'gerbera-config.json'
  });
};

const retrieveGerberaValues = (type) => {
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: {
      req_type: type,
      sid: Auth.getSessionId()
     }
  });
};

const loadConfig = (response, item) => {

console.log(`loadConfig ${item}`);
console.log(response);

  if (response.success && item in response && response[item]) {
     current_config[item] = response[item];
  }
  if (current_config.config && current_config.values) {
    const datagrid = $('#configgrid');

    if (datagrid.hasClass('grb-config')) {
      datagrid.config('destroy');
    }
    current_config.changedItems = {};
    datagrid.config({
      meta: current_config.config,
      values: current_config.values,
      addResultItem: setChangedItem,
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
  console.log(current_config.changedItems);
  const changedKeys = Object.getOwnPropertyNames(current_config.changedItems);
  if (changedKeys.length > 0 ) {
    const saveData = {
      req_type: 'config_save',
      sid: Auth.getSessionId(),
      data: [],
      changedCount: changedKeys.length,
      updates: 'check'
    };
    changedKeys.forEach((key) => {
      let i = current_config.changedItems[key];
      if (i.item) {
        saveData.data.push({
          item: i.item,
          id: i.id,
          value: i.value,
          origValue: i.origValue,
          status: i.status});
      }
    });
    console.log(saveData);

    try {
      $.ajax({
        url: GerberaApp.clientConfig.api,
        type: 'get',
        data: saveData
      })
        .then((response) => {
          menuSelected();
          Updates.showMessage('Successfully saved ' + saveData.data.length + ' config items', undefined, 'success', 'fa-check');
          console.log(response);
        })
        .catch((err) => GerberaApp.error(err));
    } catch (e) {
      console.log(e);
    }
  } else {
    console.log("Nothing to save");
  }
};

const clearConfig = () => {
  const saveData = {
    req_type: 'config_save',
    sid: Auth.getSessionId(),
    data: [],
    changedCount: -1,
    updates: 'check'
  };

  console.log(saveData);

  try {
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: saveData
    })
      .then((response) => {
        menuSelected();
        console.log(response);
      })
      .catch((err) => GerberaApp.error(err));
  } catch (e) {
    console.log(e);
  }
};

export const Config = {
  destroy,
  loadConfig,
  saveConfig,
  clearConfig,
  initialize,
  menuSelected,
};
