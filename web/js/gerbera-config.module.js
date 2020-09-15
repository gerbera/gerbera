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

const destroy = () => {
  const datagrid = $('#configgrid');
  if (datagrid.hasClass('grb-config')) {
    datagrid.config('destroy');
  } else {
    datagrid.html('');
  }
};

let current_config = { config: null, values: null };

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
    datagrid.config({
      meta: current_config.config,
      values: current_config.values,
      itemType: 'config'
    });
  }
};

export const Config = {
  destroy,
  loadConfig,
  initialize,
  menuSelected,
};
