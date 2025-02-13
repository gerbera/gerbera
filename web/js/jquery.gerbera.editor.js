/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.editor.js - this file is part of Gerbera.

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
(function ($) {
  function hideFields(fields) {
    for (let i = 0; i < fields.length; i++) {
      const field = fields[i];
      field.closest('.form-group').hide();
    }
  }

  function showFields(fields) {
    for (let i = 0; i < fields.length; i++) {
      const field = fields[i];
      field.closest('.form-group').show();
    }
  }

  const defaultClasses = {
    container: { oclass: 'object.container', type: 'container' },
    item: { oclass: 'object.item', type: 'item' },
    external_url: { oclass: 'object.item', type: 'external_url', protocol: 'http-get' },
    image: { oclass: 'object.item.imageItem', type: 'item' },
    imagePhoto: { oclass: 'object.item.imageItem.photo', type: 'item' },
    audio: { oclass: 'object.item.audioItem', type: 'item' },
    audioBroadcast: { oclass: 'object.item.audioItem.audioBroadcast', type: 'external_url', protocol: 'http-get' },
    video: { oclass: 'object.item.videoItem', type: 'item' },
    videoBroadcast: { oclass: 'object.item.videoItem.videoBroadcast', type: 'external_url', protocol: 'http-get' },
  };
  const purposeValues = [
    { label: 'Thumbnail', type: 'thumbnail' },
    { label: 'Subtitle', type: 'subtitle' },
  ];
  const objectFlags = [
    "Restricted",
    "Searchable",
    "UseResourceRef",
    "PersistentContainer",
    "PlaylistRef",
    "ProxyUrl",
    "OnlineService",
    "OggTheora",
  ];
  let resources = [];
  function addNewItem(modal, itemData) {
    const itemType = itemData.type;
    const item = itemData.item;
    const editObjectType = modal.find('#editObjectType');
    const editTitle = modal.find('#editTitle');
    const editLocation = modal.find('#editLocation');
    const editClass = modal.find('#editClass');
    const editDesc = modal.find('#editDesc');
    const editMime = modal.find('#editMime');
    const editProtocol = modal.find('#editProtocol');
    const editFlags = modal.find('#editFlags');
    const editLut = modal.find('#editLut');
    const editLmt = modal.find('#editLmt');
    const saveButton = modal.find('#editSave');
    let editFlagBox = {};
    objectFlags.forEach((flag) => {
      editFlagBox[flag] = modal.find('#editFlag-' + flag);
    });


    editObjectType.off('click').on('click', function () {
      const editObjectType = $(this);
      addNewItem(modal, {
        type: editObjectType.val(),
        item: item,
        onSave: itemData.onSave,
        onData: itemData.onData
      })
    });

    modal.find('#objectIdGroup').hide();
    modal.find('#parentIdGroup').show();
    modal.find('#addParentId').val(item.id);
    modal.find('#addParentIdTxt').text(item.id);
    modal.find('.modal-title').text('Add Item');

    saveButton.text('Add Item');
    if (itemData.onSave) {
      saveButton.off('click').on('click', itemData.onSave);
    }

    editObjectType.prop('disabled', false);
    editObjectType.prop('readonly', false);
    editObjectType.val(itemType);
    let hiddenFields = [editLmt, editLut];
    objectFlags.forEach((flag) => {
      hiddenFields.push(editFlagBox[flag]);
    });
    hideFields(hiddenFields);

    editClass.val(defaultClasses[itemType].oclass);
    modal.find('.modal-dialog').removeClass('modal-xl');
    $("#editCol").hide();
    if (defaultClasses[itemType].type === 'container') {
      showFields([editObjectType, editTitle, editClass, editFlags, editFlagBox['Searchable']]);
      hideFields([editLocation, editDesc, editMime, editProtocol]);
    } else if (defaultClasses[itemType].type === 'item') {
      showFields([editObjectType, editTitle, editLocation, editClass, editDesc, editMime, editFlags, editFlagBox['OggTheora']]);
      hideFields([editProtocol]);
    } else if (defaultClasses[itemType].type === 'external_url') {
      editProtocol.val(defaultClasses[itemType].protocol);
      showFields([editObjectType, editTitle, editLocation, editClass, editDesc, editMime, editProtocol, editFlags,
        editFlagBox['ProxyUrl'],
        /* editFlagBox['OnlineService'] */]);
      modal.find('.modal-dialog').addClass('modal-xl');
      $("#editCol").show();
      const restable = modal.find('#resdata');
      restable.html('');
      const tfoot = $('<tfoot></tfoot>');
      const trow = $('<tr></tr>');
      const th1 = $('<th></th>');
      const th2 = $('<th colspan="2"></th>');
      const addRes = $('<button>Add Resource</button>');
      addRes.off('click').on('click', function () {
        resources.push({
          index: resources.length + 1,
          changed: ['purpose', 'location', 'mime-type', 'protocol'],
          purpose: ['thumbnail'],
          'location': [''],
          'mime-type': ['image/jpg'],
          protocol: ['http-get']
        });
        addNewItem(modal, {
          type: itemData.type,
          item: item,
          onSave: itemData.onSave,
          onData: itemData.onData
        });
      });
      addRes.appendTo(th2);
      th1.appendTo(trow);
      th2.appendTo(trow);
      trow.appendTo(tfoot);

      resources.forEach((res, i) => {
        const tbody = makeDetailsHead(restable, 'Resource', `#${i + 1}`, itemData.onData);
        makePurposeSelect(tbody, 'purpose', res, purposeValues);
        makeResourceInput(tbody, ['location', 'mime-type', 'protocol'], res);
        tbody.appendTo(restable);
      });
      tfoot.addClass('itemDetailFoot');
      tfoot.appendTo(restable);
      restable.show();
    }
  }

  function makeDetailsHead(table, caption, value, eventHandler) {
    const thead = $(`<thead><tr><th>${caption}</th><th colspan="2">${value}</th></tr></thead>`);
    thead.addClass('itemDetailHead');
    thead.appendTo(table);
    const tbody = $('<tbody></tbody>');

    if (eventHandler)
      thead.click(tbody, eventHandler);
    tbody.appendTo(table);
    return tbody;
  }

  function makePurposeSelect(tbody, p, resource, values) {
    const purposeSelect = $(`<select name="${p}Select" id="${p}Select"></select>`);
    purposeSelect.change(
      function (event) {
        if (resource.changed)
          resource.changed.push(p);
        else
          resource.changed = [p];
        if (!resource[p][1])
          resource[p][1] = resource[p][0];
        resource[p][0] = event.target.value;
        resource[p][1] = event.target.value;
      });
    values.forEach((purpose) => {
      const selected = resource[p][0] === purpose.type ? 'selected="selected"' : '';
      const purposeOption = $(`<option ${selected} value="${purpose.type}">${purpose.label}</option>`);
      purposeOption.appendTo(purposeSelect);
    });
    const trow = $('<tr></tr>');
    const td1 = $(`<td class="detail-column"><span>${p}</span></td>`);
    td1.appendTo(trow);
    const td2 = $('<td colspan="2"></td>');
    purposeSelect.appendTo(td2);
    td2.appendTo(trow);
    trow.appendTo(tbody);
  }

  function makeResourceInput(tbody, properties, resource) {
    properties.forEach((p) => {
      appendMetaItem(tbody, p, resource[p][0], null, null,
        function (event) {
          if (resource.changed)
            resource.changed.push(p);
          else
            resource.changed = [p];
          if (!resource[p][1])
            resource[p][1] = resource[p][0];
          resource[p][0] = event.target.value;
        });
    });
  }

  function reset(modal) {
    resources = [];
    modal.find('#editObjectType').val('item');
    modal.find('#objectId').val('');
    modal.find('#addParentId').val('');
    modal.find('#editdObjectIdTxt').text('').closest('.form-group').hide();
    modal.find('#addParentIdTxt').text('').closest('.form-group').hide();
    modal.find('#editTitle').val('').prop('disabled', false);
    modal.find('#editLocation').val('').prop('disabled', false);
    modal.find('#editClass').val('').prop('disabled', false);
    modal.find('#editDesc').val('').prop('disabled', false);
    modal.find('#editMime').val('').prop('disabled', false);
    modal.find('#editFlags').val('').prop('disabled', true);
    modal.find('#editLmt').val('').prop('disabled', true);
    modal.find('#editLut').val('').prop('disabled', true);
    modal.find('#editProtocol').val('').prop('disabled', false);
    modal.find('#editSave').text('Save Item');
    objectFlags.forEach((flag) => {
      modal.find('#editFlag-' + flag).prop('checked', false);
    });

    modal.find('#metadata').empty();
    modal.find('#auxdata').empty();
    modal.find('#resdata').empty();
    modal.find('#mediaimage').empty();

    hideDetails(modal);
    modal.find('#mediaimage').hide();
    modal.find('#detailbutton').hide();
    modal.find('#hidebutton').hide();
  }

  function appendMetaItem(tbody, name, value, special = null, rawvalue = null, eventHandler = null) {
    let content, text;
    const nl = '\n';

    const row = $('<tr></tr>');
    content = $('<td></td>');
    content.addClass('detail-column');
    text = $('<span></span>');
    text.text(name).appendTo(content);
    content.appendTo(row);
    if (rawvalue)
      content = $('<td></td>');
    else
      content = $('<td colspan="2"></td>');

    if (eventHandler) {
      const input = $(`<input class="form-control" value="${value}"/>`);
      input.change(eventHandler);
      input.appendTo(content);
    }
    else if (value) {
      text = $('<span>' + value.replace(RegExp(nl, 'g'), '<br>') + '</span>');
      text.appendTo(content);
    }
    if (special) {
      special.appendTo(content);
    }
    content.appendTo(row);

    if (rawvalue) {
      content = $('<td></td>');
      text = $('<span>' + rawvalue.replace(RegExp(nl, 'g'), '<br>') + '</span>');
      text.appendTo(content);
      content.appendTo(row);
    }
    row.appendTo(tbody);
  }

  function showDetails(modal) {
    modal.find('.modal-dialog').addClass('modal-xl');
    $("#editCol").show();
    modal.find('#detailbutton').hide();
    modal.find('#hidebutton').show();
  }

  function hideDetails(modal) {
    modal.find('.modal-dialog').removeClass('modal-xl');
    modal.find('#detailbutton').show();
    modal.find('#hidebutton').hide();
    $("#editCol").hide();
  }

  function readFlags(modal) {
    let result = [];
    objectFlags.forEach((flag) => {
      if (modal.find('#editFlag-' + flag).is(':checked')) {
        result.push(flag);
      }
    });

    return result.join('|');
  }

  function readResources(item) {
    const data = [];
    resources.forEach((res) => {
      if (res.changed) {
        res.changed.forEach((p) => {
          const output = `resource.${res.index}.${p}`;
          item[output] = encodeURIComponent(res[p][0]);
          data.push(output)
        })
      }
    });
    item.resources = encodeURIComponent(data.join('|'));
  }

  function loadItem(modal, itemData) {
    const item = itemData.item;
    if (item) {
      if (item.image) {
        modal.find('#mediaimage').prop('src', item.image.value);
        modal.find('#mediaimage').show();
      }
      let obj_type = item.obj_type;
      if (item.class.value !== 'object.item') {
        Object.getOwnPropertyNames(defaultClasses).forEach((cls) => {
          if (item.class.value.startsWith(defaultClasses[cls].oclass)) {
            obj_type = defaultClasses[cls].type;
          }
        });
      }
      modal.find('#editObjectType').val(obj_type);
      modal.find('#editObjectType').prop('disabled', true);
      modal.find('#editObjectType').prop('readonly', true);
      modal.find('#editdObjectIdTxt').text(item.object_id).closest('.form-group').show();
      modal.find('#objectId').val(item.object_id).prop('disabled', true);

      const metatable = modal.find('#metadata');
      const detailButton = modal.find('#detailbutton');
      if (item.metadata && item.metadata.metadata.length) {
        detailButton.show();
        const tbody = makeDetailsHead(metatable, 'Metadata', '', itemData.onData);
        for (let i = 0; i < item.metadata.metadata.length; i++) {
          appendMetaItem(tbody, item.metadata.metadata[i].metaname, item.metadata.metadata[i].metavalue);
        }
      }
      if (item.flags && item.flags.value) {
        const tbody = makeDetailsHead(metatable, 'Extras', '', itemData.onData);
        appendMetaItem(tbody, "flags", item.flags.value);
        tbody.hide();
        metatable.append(tbody);
      }

      const auxtable = modal.find('#auxdata');
      if (item.auxdata && item.auxdata.auxdata.length) {
        detailButton.show();
        auxtable.show();
        const tbody = makeDetailsHead(auxtable, 'Aux Data', '', itemData.onData);
        for (let i = 0; i < item.auxdata.auxdata.length; i++) {
          appendMetaItem(tbody, item.auxdata.auxdata[i].auxname, item.auxdata.auxdata[i].auxvalue);
        }
        tbody.hide();
      } else {
        auxtable.hide();
      }

      const restable = modal.find('#resdata');
      if (item.resources && item.resources.resources.length) {
        detailButton.show();
        restable.show();
        let currentResource = { index: '???', properties: [] };
        for (let i = 0; i < item.resources.resources.length; i++) {
          if (item.resources.resources[i].resname === '----RESOURCE----') {
            currentResource = { index: item.resources.resources[i].resvalue, properties: [] };
            resources.push(currentResource);
          } else {
            currentResource.properties.push(item.resources.resources[i].resname);
            currentResource[item.resources.resources[i].resname] = [item.resources.resources[i].resvalue, item.resources.resources[i].rawvalue];
          }
        }
        resources.forEach((res, i) => {
          const tbody = makeDetailsHead(restable, 'Resource', `#${res.index}`, itemData.onData);
          if (i > 0)
            tbody.hide();
          res.properties.forEach((p) => {
            switch (p) {
              case 'image': {
                appendMetaItem(tbody, 'content', null, $(`<img width="50px" class="resourceImage" src="${res[p][0]}"/>`));
                break;
              }
              case 'link': {
                appendMetaItem(tbody, 'content', null, $(`<a href=${res[p][0]}>Open</span>`));
                break;
              }
              case 'resFile': {
                if (res.handlerType[0] === 'Exturl')
                  makeResourceInput(tbody, [p], res);
                else
                  appendMetaItem(tbody, p, res[p][0], null, res[p][1]);
                break;
              }
              default: {
                appendMetaItem(tbody, p, res[p][0], null, res[p][1]);
                break;
              }
            }
          });
        });
      } else {
        restable.hide();
      }

      loadCdsObject(modal, item);
      switch (item.obj_type) {
        case 'item': {
          loadSimpleItem(modal, item, obj_type);
          break;
        }
        case 'container': {
          loadContainer(modal, item);
          break;
        }
        case 'external_url': {
          loadExternalUrl(modal, item);
          break;
        }
      }

      modal.find('#detailbutton').off('click').on('click', itemData.onDetails);
      modal.find('#hidebutton').off('click').on('click', itemData.onHide);
      modal.find('#editSave').off('click').on('click', itemData.onSave);
      modal.find('#editSave').text('Save Item');
      modal.find('.modal-title').text('Edit Item');
    }
  }

  function toggleData(modal, target) {
    if ($(target.data[0]).is(":hidden") === true)
      $(target.data[0]).show();
    else
      $(target.data[0]).hide();
  }

  function loadCdsObject(modal, item) {
    modal.find('#editTitle')
      .val(item.title.value)
      .prop('disabled', !item.title.editable)
      .closest('.form-group').show();

    modal.find('#editClass')
      .val(item.class.value)
      .prop('disabled', true)
      .closest('.form-group').show();

    if ('last_modified' in item && item['last_modified'].value !== '') {
      modal.find('#editLmt')
        .val(item['last_modified'].value)
        .prop('disabled', true)
        .closest('.form-group').show();
    } else {
      modal.find('#editLmt')
        .val('')
        .prop('disabled', true)
        .closest('.form-group').hide();
    }
    if ('last_updated' in item && item['last_updated'].value !== '') {
      modal.find('#editLut')
        .val(item['last_updated'].value)
        .prop('disabled', true)
        .closest('.form-group').show();
    } else {
      modal.find('#editLut')
        .val('')
        .prop('disabled', true)
        .closest('.form-group').hide();
    }
    if (item.flags && item.flags.value) {
      item.flags.value.split('|').forEach((flag) => {
        flag = flag.trim();
        modal.find('#editFlag-' + flag).prop('checked', true);
      });
    }
  }

  function loadSimpleItem(modal, item, obj_type) {
    modal.find('#editLocation')
      .val(item.location.value)
      .prop('disabled', !item.location.editable)
      .closest('.form-group').show();

    modal.find('#editDesc')
      .val(item.description.value)
      .prop('disabled', !item.description.editable)
      .closest('.form-group').show();

    modal.find('#editMime')
      .val(item['mime-type'].value)
      .prop('disabled', true)
      .closest('.form-group').show();

    let visibleFields = [];
    let hiddenFields = [
      modal.find('#editProtocol'),
      modal.find('#editFlag-Searchable'),
      modal.find('#editFlag-ProxyUrl'),
      modal.find('#editFlag-OnlineService'),
      modal.find('#editFlag-Restricted'),
      modal.find('#editFlag-PersistentContainer'),
      modal.find('#editFlag-PlaylistRef'),
      modal.find('#editFlag-UseResourceRef'),
    ];
    if (obj_type === 'item' || obj_type.startsWith('video')) {
      visibleFields.push(modal.find('#editFlags'));
      visibleFields.push(modal.find('#editFlag-OggTheora'));
    } else {
      hiddenFields.push(modal.find('#editFlags'));
      hiddenFields.push(modal.find('#editFlag-OggTheora'));
    }
    showFields(visibleFields);
    hideFields(hiddenFields);
  }

  function loadContainer(modal, item) {
    modal.find('#editFlags')
      .val(item.flags.value)
      .prop('disabled', true)
      .closest('.form-group').show();

    showFields([
      modal.find('#editFlags'),
      modal.find('#editFlag-Searchable'),
      modal.find('#editFlag-PersistentContainer'),
      modal.find('#editFlag-PlaylistRef'),
    ]);
    hideFields([
      modal.find('#editLocation'),
      modal.find('#editMime'),
      modal.find('#editDesc'),
      modal.find('#editProtocol'),
      modal.find('#editFlag-ProxyUrl'),
      modal.find('#editFlag-OnlineService'),
      modal.find('#editFlag-Restricted'),
      modal.find('#editFlag-OggTheora'),
      modal.find('#editFlag-UseResourceRef'),
    ]);
  }

  function loadExternalUrl(modal, item) {
    modal.find('#editLocation')
      .val(item.location.value)
      .prop('disabled', !item.location.editable)
      .closest('.form-group').show();

    modal.find('#editDesc')
      .val(item.description.value)
      .prop('disabled', !item.description.editable)
      .closest('.form-group').show();

    modal.find('#editMime')
      .val(item['mime-type'].value)
      .prop('disabled', !item['mime-type'].editable)
      .closest('.form-group').show();

    modal.find('#editProtocol')
      .val(item.protocol.value)
      .prop('disabled', !item.protocol.editable)
      .closest('.form-group').show();

    modal.find('#editFlags')
      .val(item.flags.value)
      .prop('disabled', true)
      .closest('.form-group').show();

    showFields([
      modal.find('#editFlags'),
      modal.find('#editFlag-ProxyUrl'),
      // modal.find('#editFlag-OnlineService'),
    ]);
    hideFields([
      modal.find('#editFlag-Searchable'),
      modal.find('#editFlag-Restricted'),
      modal.find('#editFlag-PersistentContainer'),
      modal.find('#editFlag-PlaylistRef'),
      modal.find('#editFlag-OggTheora'),
      modal.find('#editFlag-UseResourceRef'),
      modal.find('#editFlag-OnlineService'),
    ]);
  }

  function saveItem(modal) {
    let item;
    const objectId = modal.find('#objectId');
    const editObjectType = modal.find('#editObjectType');
    const editTitle = modal.find('#editTitle');
    const editLocation = modal.find('#editLocation');
    const editDesc = modal.find('#editDesc');
    const editMime = modal.find('#editMime');
    const editProtocol = modal.find('#editProtocol');

    switch (defaultClasses[editObjectType.val()].type) {
      case 'item': {
        item = {
          object_id: objectId.val(),
          title: encodeURIComponent(editTitle.val()),
          description: encodeURIComponent(editDesc.val()),
          flags: readFlags(modal),
        };
        readResources(item);
        break;
      }
      case 'container': {
        item = {
          object_id: objectId.val(),
          title: encodeURIComponent(editTitle.val()),
          flags: readFlags(modal),
        };
        readResources(item);
        break;
      }
      case 'external_url': {
        item = {
          object_id: objectId.val(),
          title: encodeURIComponent(editTitle.val()),
          description: encodeURIComponent(editDesc.val()),
          location: encodeURIComponent(editLocation.val()),
          'mime-type': encodeURIComponent(editMime.val()),
          protocol: editProtocol.val(),
          flags: readFlags(modal),
        };
        readResources(item);
        break;
      }
    }
    return item;
  }

  function addObject(modal) {
    let item;
    const parentId = modal.find('#addParentId');
    const editObjectType = modal.find('#editObjectType');
    const editClass = modal.find('#editClass');
    const editTitle = modal.find('#editTitle');
    const editLocation = modal.find('#editLocation');
    const editDesc = modal.find('#editDesc');
    const editMime = modal.find('#editMime');
    const editProtocol = modal.find('#editProtocol');

    switch (defaultClasses[editObjectType.val()].type) {
      case 'item': {
        item = {
          parent_id: parentId.val(),
          obj_type: defaultClasses[editObjectType.val()].type,
          class: editClass.val(),
          title: encodeURIComponent(editTitle.val()),
          location: encodeURIComponent(editLocation.val()),
          description: encodeURIComponent(editDesc.val()),
          flags: readFlags(modal),
        };
        readResources(item);
        break;
      }
      case 'container': {
        item = {
          parent_id: parentId.val(),
          obj_type: defaultClasses[editObjectType.val()].type,
          class: editClass.val(),
          title: encodeURIComponent(editTitle.val()),
          flags: readFlags(modal),
        };
        readResources(item);
        break;
      }
      case 'external_url': {
        item = {
          parent_id: parentId.val(),
          obj_type: defaultClasses[editObjectType.val()].type,
          class: editClass.val(),
          title: encodeURIComponent(editTitle.val()),
          description: encodeURIComponent(editDesc.val()),
          location: encodeURIComponent(editLocation.val()),
          'mime-type': encodeURIComponent(editMime.val()),
          protocol: editProtocol.val(),
          flags: readFlags(modal),
        };
        readResources(item);
        break;
      }
    }
    return item;
  }

  let _super = $.fn.modal;

  // create a new constructor
  let GerberaEditorModal = function (element, options) { // eslint-disable-line
    _super.Constructor.apply(this, arguments);
  };

  GerberaEditorModal.prototype = $.extend({}, _super.Constructor.prototype, {
    constructor: GerberaEditorModal,
    _super: function () {
      let args = $.makeArray(arguments);
      _super.Constructor.prototype[args.shift()].apply(this, args);
    },
    addNewItem: function (itemData) {
      reset($(this._element));
      return addNewItem($(this._element), itemData);
    },
    addObject: function () {
      return addObject($(this._element));
    },
    reset: function () {
      return reset($(this._element));
    },
    showDetails: function () {
      return showDetails($(this._element));
    },
    hideDetails: function () {
      return hideDetails($(this._element));
    },
    loadItem: function (itemData) {
      reset($(this._element));
      return loadItem($(this._element), itemData);
    },
    saveItem: function () {
      return saveItem($(this._element));
    },
    toggleData: function (target) {
      return toggleData($(this._element), target);
    }
  });

  // override the old initialization with the new constructor
  $.fn.editmodal = $.extend(function (option) {
    let args = $.makeArray(arguments);
    let retval = null;
    option = args.shift();

    this.each(function () {
      let $this = $(this);
      let data = $this.data('modal');
      let options = $.extend({}, _super.defaults, $this.data(), typeof option === 'object' && option);

      if (!data) {
        $this.data('modal', (data = new GerberaEditorModal(this, options)));
      }
      if (typeof option === 'string') {
        // for custom methods return the method value, not the selector
        retval = data[option].apply(data, args);
      } else if (options.show) {
        data.show.apply(data, args);
      }
    });

    if (!retval) {
      retval = this;
    }
    return retval;
  }, $.fn.editmodal)
})(jQuery);
