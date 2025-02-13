/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-items.module.js - this file is part of Gerbera.

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
import { Trail } from './gerbera-trail.module.js';
import { Updates } from "./gerbera-updates.module.js";

const destroy = () => {
  const datagrid = $('#datagrid');
  if (datagrid.hasClass('grb-dataitems')) {
    datagrid.dataitems('destroy');
  } else {
    datagrid.html('');
  }
};

let currentItemView = undefined;

export class ItemView {
  parentId = -1;
  start = 0;
  count = 10;
  action = 'browse';
  constructor(pId, act) {
    this.parentId = pId;
    this.action = act;
    $('#search').hide();
  }
  get requestData() {
    let result = {
      parent_id: this.parentId,
      action: this.action,
      req_type: 'items',
      start: this.start,
      count: this.count,
      select_it: 0
    };
    result[Auth.SID] = Auth.getSessionId();
    return result;
  }

  retrieveGerberaItems(pId = -1) {
    if (pId === -1)
      this.parentId = GerberaApp.currentTreeItem.gerbera.id;
    else
      this.parentId = pId;
    this.count = GerberaApp.viewItems();
    const data = this.requestData; // ensure call sequence
    return $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: data
    });
  }

  dataItems(response) {
    const items = Items.transformItems(response.items.item);
    setPage(1); // reset page
    let pager = {
      currentPage: Math.ceil(response.items.start / GerberaApp.viewItems()) + 1,
      pageCount: 10,
      gridMode: GerberaApp.gridMode(),
      onClick: Items.retrieveItemsForPage,
      onNext: Items.nextPage,
      onPrevious: Items.previousPage,
      onItemsPerPage: Items.changeItemsPerPage,
      onModeSelect: Items.changeGridMode,
      totalMatches: response.items.total_matches,
      itemsPerPage: GerberaApp.viewItems(),
      ippOptions: GerberaApp.itemsPerPage(),
      parentId: response.items.parent_id
    };
    return {
      data: items,
      pager: pager,
      onEdit: Items.editItem,
      onDelete: Items.deleteItemFromList,
      onDownload: Items.downloadItem,
      onAdd: Items.addFileItem,
      itemType: GerberaApp.getType(),
      parentItem: response.items
    }
  }
}

export class BrowseItemView extends ItemView {
  constructor(pId) {
    super(pId, 'browse');
    $('#datagrid').show();
  }
  get requestData() {
    let result = super.requestData;
    result.updates = 'check';
    return result;
  }
}

export class SearchItemView extends ItemView {
  searchCriteria = '';
  sortCriteria = '';
  searchableContainers = false;

  constructor(pId) {
    super(pId, 'search');
    this.searchGerberaItems = this.searchGerberaItems.bind(this);
    $('#search').show();
    $('#datagrid').hide();
    $('#searchButton').off('click').on('click', this.searchGerberaItems);

    $('#searchCaps').text(GerberaApp.serverConfig.searchCaps.split(',').join(', '));
    $('#sortCaps').text(GerberaApp.serverConfig.sortCaps.split(',').join(', '));
    const search = GerberaApp.getSearch();
    $('#searchQuery').val(search[0]);
    $('#searchSort').val(search[1]);
  }

  get requestData() {
    let result = super.requestData;
    result.searchCriteria = this.searchCriteria;
    result.sortCriteria = this.sortCriteria;
    result.searchableContainers = this.searchableContainers;
    return result;
  }

  searchGerberaItems() {
    this.searchCriteria = $('#searchQuery').val();
    this.sortCriteria = $('#searchSort').val();
    GerberaApp.setSearch(this.searchCriteria, this.sortCriteria);
    $('#datagrid').show();

    this.retrieveGerberaItems()
      .then((response) => Items.loadItems(response))
      .catch((err) => GerberaApp.error(err));
  }
}

export class FileItemView extends ItemView {
  constructor(pId) {
    super(pId, 'files');
    $('#datagrid').show();
  }
  get requestData() {
    let result = super.requestData;
    result.req_type = 'files';
    return result;
  }
  dataItems(response) {
    return {
      data: Items.transformFiles(response.files.file),
      pager: undefined,
      onEdit: Items.editItem,
      onDelete: Items.deleteItemFromList,
      onDownload: Items.downloadItem,
      onAdd: Items.addFileItem,
      itemType: GerberaApp.getType(),
      parentItem: response.files
    }
  }
}

const initialize = () => {
  $('#datagrid').html('');
  $('#search').hide();
  return Promise.resolve();
};

const viewFactory = function (data) {
  GerberaApp.currentTreeItem = data;
  switch (GerberaApp.getType()) {
    case 'search':
      currentItemView = new SearchItemView(data.gerbera.id);
      return undefined;
    case 'db':
      currentItemView = new BrowseItemView(data.gerbera.id);
      break;
    default:
      currentItemView = new FileItemView(data.gerbera.id);
      break;
  }
  return currentItemView;
};

const treeItemSelected = function (data) {
  if (viewFactory(data))
    currentItemView.retrieveGerberaItems()
      .then((response) => loadItems(response, data.gerbera))
      .catch((err) => GerberaApp.error(err));
};

const loadItems = (response, item) => {
  if (response.success) {
    const dataItems = currentItemView.dataItems(response);
    const datagrid = $('#datagrid');

    if (datagrid.hasClass('grb-dataitems')) {
      datagrid.dataitems('destroy');
    }
    datagrid.dataitems(dataItems);
    Trail.makeTrailFromItem(dataItems.parentItem, item);
  }
};

const setPage = (pageNumber) => {
  GerberaApp.setCurrentPage(pageNumber);
};

const nextPage = (event) => {
  const pageItem = event.data;
  const pageNumber = GerberaApp.currentPage() + ((pageItem.increment) ? pageItem.increment : 1);
  const itemsPerPage = pageItem.gridMode === 3 ? 1 : pageItem.itemsPerPage;
  const start = (pageNumber * itemsPerPage) - itemsPerPage;
  if (start < pageItem.totalMatches) {
    const pageEvent = {
      data: {
        pageNumber: pageNumber,
        itemsPerPage: pageItem.itemsPerPage,
        parentId: pageItem.parentId
      }
    };
    return retrieveItemsForPage(pageEvent)
  } else {
    return Promise.resolve();
  }
};

const changeItemsPerPage = (pageItem, newValue) => {
  GerberaApp.setViewItems(newValue);
  const pageEvent = {
    data: {
      pageNumber: 1,
      itemsPerPage: newValue,
      gridMode: pageItem.gridMode,
      parentId: pageItem.parentId
    }
  };
  return retrieveItemsForPage(pageEvent)
};

const changeGridMode = (pageItem, newValue) => {
  pageItem.itemsPerPage = GerberaApp.setGridMode(newValue);
  const pageEvent = {
    data: {
      pageNumber: (pageItem.gridMode === 3 && pageItem.gridMode != newValue) ? 1 : pageItem.pageNumber,
      itemsPerPage: pageItem.itemsPerPage,
      gridMode: newValue,
      parentId: pageItem.parentId
    }
  };
  return retrieveItemsForPage(pageEvent)
};

const previousPage = function (event) {
  const pageItem = event.data;
  const pageNumber = GerberaApp.currentPage() - ((pageItem.increment) ? pageItem.increment : 1);
  const itemsPerPage = pageItem.gridMode === 3 ? 1 : pageItem.itemsPerPage;
  const start = (pageNumber * itemsPerPage) - itemsPerPage;
  if (start >= 0) {
    const pageEvent = {
      data: {
        pageNumber: pageNumber,
        itemsPerPage: pageItem.itemsPerPage,
        parentId: pageItem.parentId
      }
    };
    return retrieveItemsForPage(pageEvent)
  } else {
    return Promise.resolve();
  }
};

const editItem = (event) => {
  const item = event.data;
  if (item) {
    var requestData = {
      req_type: 'edit_load',
      object_id: item.id,
      updates: 'check'
    };
    requestData[Auth.SID] = Auth.getSessionId();
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    })
      .then((response) => loadEditItem(response))
      .catch((err) => GerberaApp.error(err));
  }
};

const deleteItemFromList = (event) => {
  const item = event.data;
  if (item) {
    deleteGerberaItem(item)
      .then((response) => deleteComplete(response))
      .catch((err) => GerberaApp.error(err));
  }
};

const deleteGerberaItem = (item, includeAll) => {
  const deleteAll = includeAll ? 1 : 0;
  var requestData = {
    req_type: 'remove',
    object_id: item.id,
    all: deleteAll,
    updates: 'check'
  };
  requestData[Auth.SID] = Auth.getSessionId();
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: requestData
  });
};

const deleteComplete = (response) => {
  if (response.success && GerberaApp.currentTreeItem) {
    // TODO: better served as an event to refresh
    Updates.showMessage('Successfully removed item', undefined, 'success', 'fa-check');
    Items.treeItemSelected(GerberaApp.currentTreeItem);
    Updates.updateTreeByIds(response);
  } else {
    GerberaApp.error('Failed to delete item');
  }
};

const downloadItem = (event) => {
  const item = event.data;
  event.preventDefault();

  var link = document.createElement("a");
  link.download = item.text;
  link.type = item.mtype;
  link.href = item.url;

  document.body.appendChild(link);
  link.click();

  document.body.removeChild(link);
};

const addFileItem = (event) => {
  const item = event.data;
  if (item) {
    var requestData = {
      req_type: 'add',
      object_id: item.id
    };
    requestData[Auth.SID] = Auth.getSessionId();
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    })
      .then((response) => addFileItemComplete(response))
      .catch((err) => GerberaApp.error(err));
  }
};

const addFileItemComplete = (response) => {
  if (response.success) {
    Updates.showMessage('Successfully added item', undefined, 'success', 'fa-check');
    Updates.addUiTimer();
  } else {
    GerberaApp.error('Failed to add item');
  }
};

const addVirtualItem = (event) => {
  const item = event.data;
  const editModal = $('#editModal');
  if (item) {
    editModal.editmodal('addNewItem',
      {
        type: 'container',
        item: item,
        onSave: addObject,
        onData: toggleData,
      });
    editModal.editmodal('show');
  }
};

const addObject = () => {
  const item = $('#editModal').editmodal('addObject');
  const addObjectData = {
    req_type: 'add_object',
    updates: 'check'
  };
  addObjectData[Auth.SID] = Auth.getSessionId();
  const requestData = $.extend({}, item, addObjectData);

  if (requestData.parent_id && requestData.parent_id >= 0) {
    return $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    })
      .then((response) => addObjectComplete(response))
      .catch((err) => GerberaApp.error(err));
  }
};

const addObjectComplete = function (response) {
  if (response.success && GerberaApp.currentTreeItem) {
    // TODO: better served as an event to refresh
    let editModal = $('#editModal');
    editModal.editmodal('hide');
    editModal.editmodal('reset');
    Updates.showMessage('Successfully added object', undefined, 'success', 'fa-check');
    Items.treeItemSelected(GerberaApp.currentTreeItem);
    Updates.updateTreeByIds(response);
  } else {
    GerberaApp.error('Failed to add object');
  }
};

const loadEditItem = (response) => {
  const editModal = $('#editModal');
  if (response.success) {
    editModal.editmodal('loadItem', {
      item: response.item,
      onSave: saveItem,
      onDetails: showDetails,
      onHide: hideDetails,
      onData: toggleData,
    });

    editModal.editmodal('show');
    Updates.updateTreeByIds(response);
  }
};

const transformItems = (items) => {
  const widgetItems = [];
  for (let i = 0; i < items.length; i++) {
    const gItem = items[i];
    const item = {
      id: gItem.id,
      text: gItem.title,
      url: gItem.res,
      mtype: ('mtype' in gItem) ? gItem.mtype : null,
      upnp_class: ('upnp_class' in gItem) ? gItem.upnp_class : null,
      image: ('image' in gItem) ? gItem.image : null,
      index: ('index' in gItem) ? gItem.index : null,
      part: ('part' in gItem) ? gItem.part : null,
      track: ('track' in gItem) ? gItem.track : null,
      size: ('size' in gItem) ? gItem.size : null,
      duration: ('duration' in gItem) ? gItem.duration : null,
      resolution: ('resolution' in gItem) ? gItem.resolution : null,
    };

    if (!GerberaApp.serverConfig.enableNumbering) {
      item.part = null;
      item.track = null;
      item.index = null;
    }
    if (!GerberaApp.serverConfig.enableThumbnail) {
      item.image = null;
    }
    if (GerberaApp.serverConfig.enableVideo) {
      item.video = '/content/media/object_id/' + gItem.id + '/res_id/0/ext/file.mp4';
    }

    widgetItems.push(item);
  }
  return widgetItems;
};

const transformFiles = (files) => {
  const widgetItems = [];
  for (let i = 0; i < files.length; i++) {
    const gItem = files[i];
    const item = {
      id: gItem.id,
      text: gItem.filename
    };
    widgetItems.push(item);
  }
  return widgetItems;
};

const retrieveItemsForPage = (event) => {
  const pageItem = event.data;
  currentItemView.parentId = pageItem.parentId;
  currentItemView.count = pageItem.gridMode === 3 ? 1 : pageItem.itemsPerPage;
  currentItemView.start = (pageItem.pageNumber * currentItemView.count) - currentItemView.count;
  return currentItemView.retrieveGerberaItems()
    .then((response) => loadItems(response))
    .then(() => {
      setPage(pageItem.pageNumber)
    })
    .catch((err) => GerberaApp.error(err))
};

const saveItem = () => {
  const item = $('#editModal').editmodal('saveItem');
  const saveData = {
    req_type: 'edit_save',
    updates: 'check'
  };
  saveData[Auth.SID] = Auth.getSessionId();
  const requestData = $.extend({}, item, saveData);

  if (requestData.object_id && requestData.object_id > 0) {
    $.ajax({
      url: GerberaApp.clientConfig.api,
      type: 'get',
      data: requestData
    })
      .then((response) => saveItemComplete(response))
      .catch((err) => GerberaApp.error(err))
  }
};

const toggleData = (event) => {
  $('#editModal').editmodal('toggleData', event);
};

const showDetails = () => {
  $('#editModal').editmodal('showDetails');
};

const hideDetails = () => {
  $('#editModal').editmodal('hideDetails');
};

const saveItemComplete = (response) => {
  if (response.success && GerberaApp.currentTreeItem) {
    // TODO: better served as an event to refresh
    const editModal = $('#editModal');
    editModal.editmodal('hide');
    editModal.editmodal('reset');
    Updates.showMessage('Successfully saved item', undefined, 'success', 'fa-check');
    Updates.updateTreeByIds(response);
  } else {
    GerberaApp.error('Failed to save item');
  }
};

export const Items = {
  addFileItem,
  addFileItemComplete,
  addObject,
  addVirtualItem,
  deleteComplete,
  deleteItemFromList,
  deleteGerberaItem,
  destroy,
  downloadItem,
  editItem,
  loadEditItem,
  loadItems,
  initialize,
  transformFiles,
  transformItems,
  retrieveItemsForPage,
  saveItem,
  saveItemComplete,
  setPage,
  changeItemsPerPage,
  changeGridMode,
  nextPage,
  previousPage,
  treeItemSelected,
  viewFactory,
};
