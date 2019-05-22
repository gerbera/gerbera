/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-updates.module.js - this file is part of Gerbera.

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
import {Auth} from "./gerbera-auth.module.js";
import {GerberaApp} from "./gerbera-app.module.js";
import {Tree} from "./gerbera-tree.module.js";

let POLLING_INTERVAL;
let UI_TIMEOUT;

const initialize = () => {
  $('#toast').toast();
  $(document).ajaxComplete(errorCheck);
  return Promise.resolve();
};

const errorCheck = (event, xhr) => {
  const response = xhr.responseJSON;
  if (response && !response.success) {
    if (response.error) {
      showMessage(response.error.text, undefined, 'danger', 'fa-exclamation-triangle');
      if(response.error.code === '900') {
        GerberaApp.disable();
      } else if(response.error.code === '400') {
        Auth.handleLogout();
      }
    }
  }
};

const showMessage = (message, callback, type, icon) => {
  const toast = {message: message, type: type, icon: icon};
  if (callback) {
    toast.callback = callback;
  }
  $('#toast').toast('show', toast);
};

const showTask = (message, callback, type, icon) => {
  const toast = {message: message, type: type, icon: icon};
  if (callback) {
    toast.callback = callback;
  }
  $('#toast').toast('showTask', toast);
};

const getUpdates = (force) => {
  if (GerberaApp.isLoggedIn()) {
    let requestData = {
      req_type: 'void',
      sid: Auth.getSessionId()
    };

    let checkUpdates;
    if (GerberaApp.getType() !== 'db') {
      checkUpdates = {};
    } else {
      checkUpdates = {
        updates: (force ? 'get' : 'check')
      };
    }
    requestData = $.extend({}, requestData, checkUpdates);

    return $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    })
      .then((response) => Updates.updateTask(response))
      .then((response) => Updates.updateUi(response))
      .catch((response) => Updates.clearAll(response))
  } else {
    return Promise.resolve();
  }
};

const updateTask = (response) => {
  let promise;
  if (response.success) {
    if (response.task) {
      const taskId = response.task.id;
      if (taskId === -1) {
        promise = Updates.clearTaskInterval(response);
      } else {
        showTask(response.task.text, undefined, 'info', 'fa-refresh fa-spin fa-fw');
        Updates.addTaskInterval();
        promise = Promise.resolve(response);
      }
    } else {
      promise = Updates.clearTaskInterval(response)
    }
  } else {
    promise = Promise.resolve(response);
  }
  return promise;
};

const updateUi = (response) => {
  if (response.success) {
    updateTreeByIds(response);
  }
  return Promise.resolve(response);
};

const clearTaskInterval = (response) => {
  if (Updates.isPolling()) {
    window.clearInterval(POLLING_INTERVAL);
    POLLING_INTERVAL = false;
  }
  return Promise.resolve(response);
};

const clearUiTimer = (response) => {
  if (Updates.isTimer()) {
    window.clearTimeout(UI_TIMEOUT);
    UI_TIMEOUT = false;
  }
  return Promise.resolve(response);
};

const addUiTimer = (interval) => {
  const timeoutInterval = interval || GerberaApp.serverConfig['poll-interval'];
  if (!Updates.isTimer()) {
    UI_TIMEOUT = window.setTimeout(function () {
      Updates.getUpdates(true);
    }, timeoutInterval);
  }
};

const addTaskInterval = () => {
  if (!Updates.isPolling()) {
    POLLING_INTERVAL = window.setInterval(function () {
      Updates.getUpdates(false);
    }, GerberaApp.serverConfig['poll-interval']);
  }
};

const isTimer = () => {
  return UI_TIMEOUT;
};

const isPolling = () => {
  return POLLING_INTERVAL;
};

const clearAll = (response) => {
  Updates.clearUiTimer(response);
  Updates.clearTaskInterval(response);
};

const updateTreeByIds = (response) => {
  if (response && response.success) {
    if (response.update_ids) {
      const updateIds = response.update_ids;
      if (updateIds.pending) {
        Updates.addUiTimer(800);
      } else if (updateIds.updates === false) {
        Updates.clearUiTimer(response);
      } else {
        if (updateIds.ids === 'all') {
          Tree.reloadTreeItemById('0');
          Updates.clearUiTimer();
        } else if (updateIds.ids && updateIds.ids.length > 0) {
          var idList = updateIds.ids.split(',');
          if ($.inArray('0', idList) > -1) {
            Tree.reloadTreeItemById('0');
          } else {
            for (let i = 0; i < idList.length; i++) {
              let idToReload = idList[i];
              Tree.reloadParentTreeItem(idToReload);
            }
          }
          Updates.clearUiTimer();
        } else {
          Updates.addUiTimer(800);
        }
      }
    } else {
      Updates.clearUiTimer();
    }
  }
};

export const Updates = {
  addUiTimer,
  addTaskInterval,
  clearAll,
  clearTaskInterval,
  clearUiTimer,
  errorCheck,
  getUpdates,
  initialize,
  isPolling,
  isTimer,
  showMessage,
  updateTask,
  updateTreeByIds,
  updateUi,
  POLLING_INTERVAL,
  UI_TIMEOUT
};