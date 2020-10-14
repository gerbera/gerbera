/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-tweak.module.js - this file is part of Gerbera.

    Copyright (C) 2020 Gerbera Contributors

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
import {Updates} from './gerbera-updates.module.js';

const initialize = () => {
  $('#dirTweakModal').dirtweakmodal('reset');
};

const addDirTweak = (event) => {
  const item = event.data;
  if (item) {
    let requestData = {
      req_type: 'config_load',
      sid: Auth.getSessionId(),
    };

    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    }).then((response) => {
      loadNewDirTweak(response, item.fullPath);
    }).catch((err) => {
      GerberaApp.error(err);
    });
  }
};

const loadNewDirTweak = (response, path) => {
  const dirtweakModal = $('#dirTweakModal');
  if (response.success) {
    dirtweakModal.dirtweakmodal('loadItem', {values: response.values, path: path, onSave: submitDirTweak});
    dirtweakModal.dirtweakmodal('show');
  }
};

const submitDirTweak = () => {
  const dirtweakModal = $('#dirTweakModal');
  const item = dirtweakModal.dirtweakmodal('saveItem');

  const saveData = {
    req_type: 'config_save',
    sid: Auth.getSessionId(),
    data: [],
    changedCount: 0,
    updates: 'check'
  };
  saveData.data.push({
    item: `/import/directories/tweak[${item.index}]`,
    id: item.id,
    value: '',
    origValue: '',
    status: item.status});
  Object.getOwnPropertyNames(item).forEach((key) => {
      if (key != "id" && key != "status" && key != "index") {
        saveData.data.push({
          item: `/import/directories/tweak[${item.index}]/attribute::${key}`,
          id: item.id,
          value: item[key].toString(),
          origValue: '',
          status: 'changed'});
      }
  });
  saveData.changedCount = saveData.data.length;
console.log(saveData);

  if (item) {
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: saveData
    }).then((response) => {
      submitDirTweakComplete(response);
    }).catch((err) => {
      GerberaApp.error(err);
    });
  }
};

const submitDirTweakComplete = (response) => {
  let msg;
  if (response.success) {
    if (response.task) {
      msg = response.task.text;
    } else {
      msg = 'Successfully submitted dirtweak';
    }
    Updates.showMessage(msg, undefined, 'success', 'fa-check');
    Updates.getUpdates(false);
  } else {
    GerberaApp.error('Failed to submit dirtweak');
  }
};

export const Tweaks = {
  addDirTweak,
  initialize,
  loadNewDirTweak,
  submitDirTweak,
  submitDirTweakComplete,
};
