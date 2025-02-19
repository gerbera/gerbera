/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-autoscan.module.js - this file is part of Gerbera.

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
import { GerberaApp } from './gerbera-app.module.js';
import { Auth } from './gerbera-auth.module.js';
import { Updates } from './gerbera-updates.module.js';

const initialize = () => {
  $('#autoscanModal').autoscanmodal('reset');
};

const addAutoscan = (event) => {
  const item = event.data;
  const fromFs = GerberaApp.getType() === 'fs';
  if (item) {
    let requestData = {
      req_type: 'autoscan',
      object_id: item.id,
      action: 'as_edit_load',
      audio: item.audio,
      retryCount: item.retryCount,
      interval: item.interval,
      dirTypes: item.dirTypes,
      forceRescan: item.forceRescan,
      audioMusic: item.audioMusic,
      audioBook: item.audioBook,
      audioBroadcast: item.audioBroadcast,
      image: item.image,
      imagePhoto: item.imagePhoto,
      video: item.video,
      videoMovie: item.videoMovie,
      videoTV: item.videoTV,
      videoMusicVideo: item.videoMusicVideo,
      ctAudio: item.ctAudio,
      ctImage: item.ctImage,
      ctVideo: item.ctVideo,
      from_fs: fromFs,
    };
    requestData[Auth.SID] = Auth.getSessionId();

    if (GerberaApp.getType() === 'db') {
      requestData = $.extend({}, requestData, { updates: 'check' });
    }

    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    }).then((response) => {
      loadNewAutoscan(response);
    }).catch((err) => {
      GerberaApp.error(err);
    });
  }
};

const loadNewAutoscan = (response) => {
  const autoscanModal = $('#autoscanModal');
  if (response.success) {
    autoscanModal.autoscanmodal('loadItem', {
      item: response.autoscan,
      onSave: submitAutoscan,
      onDetails: showDetails,
      onHide: hideDetails
    });
    autoscanModal.autoscanmodal('show');
    Updates.updateTreeByIds(response);
  }
};

const submitAutoscan = () => {
  const autoscanModal = $('#autoscanModal');
  const item = autoscanModal.autoscanmodal('saveItem');
  const request = {
    req_type: 'autoscan',
    action: 'as_edit_save'
  };
  request[Auth.SID] = Auth.getSessionId();
  const requestData = $.extend({}, request, item);

  if (item) {
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    }).then((response) => {
      submitComplete(response);
    }).catch((err) => {
      GerberaApp.error(err);
    });
  }
};

const runScan = (event) => {
  const item = event.data;
  if (item) {
    const fromFs = GerberaApp.getType() === 'fs';
    let requestData = {
      req_type: 'autoscan',
      object_id: item.id,
      action: 'as_run',
      from_fs: fromFs,
    }
    requestData[Auth.SID] = Auth.getSessionId();
    if (GerberaApp.getType() === 'db') {
      requestData = $.extend({}, requestData, { updates: 'check' });
    }

    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    }).then((response) => {
      submitComplete(response);
    }).catch((err) => {
      GerberaApp.error(err);
    });
  }
};

const showDetails = () => {
  $('#autoscanModal').editmodal('showDetails');
};

const hideDetails = () => {
  $('#autoscanModal').editmodal('hideDetails');
};

const submitComplete = (response) => {
  let msg;
  if (response.success) {
    if (response.task) {
      msg = response.task.text;
    } else {
      msg = 'Successfully submitted autoscan';
    }
    Updates.showMessage(msg, undefined, 'success', 'fa-check');
    Updates.getUpdates(false);
  } else {
    GerberaApp.error('Failed to submit autoscan');
  }
};

export const Autoscan = {
  addAutoscan,
  runScan,
  initialize,
  loadNewAutoscan,
  submitAutoscan,
  submitComplete,
};
