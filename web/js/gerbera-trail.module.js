/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-trail.module.js - this file is part of Gerbera.

    Copyright (C) 2016-2021 Gerbera Contributors

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
import {Autoscan} from "./gerbera-autoscan.module.js";
import {Config} from "./gerbera-config.module.js";
import {GerberaApp} from "./gerbera-app.module.js";
import {Items} from "./gerbera-items.module.js";
import {Tree} from "./gerbera-tree.module.js";
import {Tweaks} from "./gerbera-tweak.module.js";
import {Updates} from "./gerbera-updates.module.js";

const destroy = () => {
  const trail = $('#trail');
  if (trail.hasClass('grb-trail')) {
    trail.trail('destroy');
  } else {
    trail.html('');
  }
};

const initialize = () => {
  $('#trail').html('');
  return Promise.resolve();
};

const makeTrail = (selectedItem, config) => {
  const items = (selectedItem !== null) ? gatherTrail(selectedItem) : [{text: "current configuration"}];
  const configDefaults = {
    itemType: GerberaApp.getType()
  };
  const trailConfig = $.extend({}, configDefaults, config);
  createTrail(items, trailConfig);
};

const gatherTrail = (treeElement) => {
  const items = [];
  let lastItem = {};
  if ($(treeElement).data('grb-id') !== undefined) {
    const title = $(treeElement).children('span.folder-title').text();
    lastItem = {
      id: $(treeElement).data('grb-id'),
      text: title,
      fullPath: "/" + title
    };
    items.push(lastItem);
  }

  $(treeElement).parents('ul li').each(function (index, element) {
    const title = $(element).children('span.folder-title').text();
    const gerberaId = $(element).data('grb-id');
    const item = {
      id: gerberaId,
      text: title
    };
    if (gerberaId != 0) {
      lastItem.fullPath = "/" + title + lastItem.fullPath;
    }
    items.push(item);
  });
  return items.reverse();
};

const createTrail = (items, config) => {
  destroy();
  $('#trail').trail({
    data: items,
    config: config
  });
};

const makeTrailFromItem = (items) => {
  const itemType = GerberaApp.getType();
  const treeElement = (itemType !== 'config') ? Tree.getTreeElementById(items.parent_id) : null;
  let enableAdd = false;
  let enableEdit = false;
  let enableDelete = false;
  let enableDeleteAll = false;
  let enableAddAutoscan = false;
  let enableEditAutoscan = false;
  let enableAddTweak = false;
  let enableConfig = false;
  let enableClearConfig = false;
  let onAdd;
  let onDelete;
  let onEdit;
  let onSave;
  let onClear;
  let onAddAutoscan;
  let onEditAutoscan;
  let onAddTweak;
  let onDeleteAll;
  let onRescan;
  const noOp = function () { return false };

  if (itemType === 'db') {
    if (items.parent_id === 0) {
      enableAdd = true;
    } else if (items.parent_id === 1) {
      enableEdit = true;
      enableAddAutoscan = true;
      enableAddTweak = true;
    } else {
      const isVirtual = ('virtual' in items) && items.virtual === true;
      const isNotProtected = ('protect_container' in items) && items.protect_container !== true;
      const allowsAutoscan = ('autoscan_type' in items) && items.autoscan_type !== 'none';
      enableAdd = isVirtual;
      enableEdit = isVirtual;
      enableDelete = isNotProtected;
      enableDeleteAll = isNotProtected && isVirtual;
      enableEditAutoscan = allowsAutoscan && isNotProtected;
    }
  } else if (itemType === 'fs') {
    enableAddTweak = items.parent_id !== 0;
    enableAddAutoscan = items.parent_id !== 0;
    enableAdd = items.parent_id !== 0;
  } else if (itemType === 'config') {
    enableConfig = true;
    enableClearConfig = GerberaApp.configMode() == 'expert';
  }

  onAdd = enableAdd ? addItem : noOp;
  onEdit = enableEdit ? editItem : noOp;
  onDelete = enableDelete ? deleteItem : noOp;
  onDeleteAll = enableDeleteAll ? deleteAllItems : noOp;
  onAddAutoscan = enableAddAutoscan ? addAutoscan : noOp;
  onEditAutoscan = enableEditAutoscan ? addAutoscan : noOp;
  onAddTweak = enableAddTweak ? addTweak : noOp;
  onSave = enableConfig ? saveConfig : noOp;
  onClear = enableClearConfig  ? clearConfig : noOp;
  onRescan = enableConfig ? reScanLibrary : noOp;

  const config = {
    enableAdd: enableAdd,
    onAdd: onAdd,
    enableEdit: enableEdit,
    onEdit: onEdit,
    enableDelete: enableDelete,
    onDelete: onDelete,
    enableDeleteAll: enableDeleteAll,
    onDeleteAll: onDeleteAll,
    enableAddAutoscan: enableAddAutoscan,
    onAddAutoscan: onAddAutoscan,
    enableEditAutoscan: enableEditAutoscan,
    onEditAutoscan: onEditAutoscan,
    onAddTweak: onAddTweak,
    enableAddTweak: enableAddTweak,
    enableConfig: enableConfig,
    enableClearConfig: enableClearConfig,
    onSave: onSave,
    onClear: onClear,
    onRescan: onRescan
  };

  makeTrail(treeElement, config)
};

const addItem = (event) => {
  var itemType = GerberaApp.getType()
  if (itemType === 'fs') {
    Items.addFileItem(event);
  } else if (itemType === 'db') {
    Items.addVirtualItem(event);
  }
};

const editItem = (event) => {
  return Items.editItem(event);
};

const addAutoscan = (event) => {
  Autoscan.addAutoscan(event);
};

const addTweak = (event) => {
  Tweaks.addDirTweak(event);
};

const saveConfig = (event) => {
  Config.saveConfig(event);
};

const clearConfig = (event) => {
  Config.clearConfig(event);
};

const reScanLibrary = (event) => {
  Config.reScanLibrary(event);
};

const deleteItem = (event) => {
  const item = event.data;
  return Items.deleteGerberaItem(item)
    .then((response) => {
      if (response.success) {
        Updates.showMessage('Successfully removed item', undefined, 'success', 'fa-check');
        Updates.updateTreeByIds(response);
      } else {
        GerberaApp.error('Failed to remove item');
      }
    })
    .catch((err) => { // eslint-disable-line
      GerberaApp.error('Failed to remove item');
    });
};

const deleteAllItems = (event) => {
  const item = event.data;
  if ('fullPath' in item) {
    delete item.fullPath;
  }
  return Items.deleteGerberaItem(item, true)
    .then((response) => {
      if (response.success) {
        Updates.showMessage('Successfully removed all items', undefined, 'success', 'fa-check');
        Updates.updateTreeByIds(response);
      } else {
        GerberaApp.error('Failed to remove all items');
      }
    })
    .catch((err) => { // eslint-disable-line
      GerberaApp.error('Failed to remove item');
    });
};

export const Trail = {
  createTrail,
  deleteAllItems,
  deleteItem,
  destroy,
  addItem,
  addAutoscan,
  addTweak,
  saveConfig,
  clearConfig,
  gatherTrail,
  initialize,
  makeTrail,
  makeTrailFromItem,
};
