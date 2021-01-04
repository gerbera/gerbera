/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.editor.js - this file is part of Gerbera.

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
(function ($) {
  function hideFields (fields) {
    for (let i = 0; i < fields.length; i++) {
      const field = fields[i];
      field.closest('.form-group').hide();
    }
  }

  function showFields (fields) {
    for (let i = 0; i < fields.length; i++) {
      const field = fields[i];
      field.closest('.form-group').show();
    }
  }

  function addNewItem (modal, itemData) {
    const itemType = itemData.type;
    const item = itemData.item;
    const editObjectType = modal.find('#editObjectType');
    const editTitle = modal.find('#editTitle');
    const editLocation = modal.find('#editLocation');
    const editClass = modal.find('#editClass');
    const editDesc = modal.find('#editDesc');
    const editMime = modal.find('#editMime');
    const editProtocol = modal.find('#editProtocol');
    const saveButton = modal.find('#editSave');

    reset(modal);

    editObjectType.off('click').on('click', function () {
      const editObjectType = $(this);
      addNewItem(modal, {type: editObjectType.val(), item: item, onSave: itemData.onSave})
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

    if (itemType === 'container') {
      editClass.val('object.container');
      showFields([editObjectType, editTitle, editClass]);
      hideFields([editLocation, editDesc, editMime, editProtocol]);
    } else if (itemType === 'item') {
      editClass.val('object.item');
      showFields([editObjectType, editTitle, editLocation, editClass, editDesc, editMime]);
      hideFields([editProtocol]);
    } else if (itemType === 'external_url') {
      editClass.val('object.item');
      editProtocol.val('http-get');
      showFields([editObjectType, editTitle, editLocation, editClass, editDesc, editMime, editProtocol]);
    }
  }

  function reset (modal) {
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
    modal.find('#editProtocol').val('').prop('disabled', false);
    modal.find('#editSave').text('Save Item');

    modal.find('#metadata').empty();
    modal.find('#auxdata').empty();
    modal.find('#resdata').empty();

    hideDetails(modal);
    modal.find('#detailbutton').hide();
    modal.find('#hidebutton').hide();
  }

  function appendMetaItem(tbody, name, value) {
    let row, content, text;
    row = $('<tr></tr>');
    content = $('<td></td>');
    content.addClass('detail-column');
    text = $('<span></span>');
    text.text(name).appendTo(content);
    row.append(content);
    content = $('<td></td>');
    text = $('<span></span>');
    text.text(value).appendTo(content);
    row.append(content);
    tbody.append(row);
  }

  function showDetails(modal) {
    modal.find('.modal-dialog').addClass('details-visible');
    modal.find('#metadata').show();
    modal.find('#auxdata').show();
    modal.find('#resdata').show();
    modal.find('#detailbutton').hide();
    modal.find('#hidebutton').show();
  }

  function hideDetails(modal) {
    modal.find('.modal-dialog').removeClass('details-visible');
    modal.find('#detailbutton').show();
    modal.find('#hidebutton').hide();
    modal.find('#metadata').hide();
    modal.find('#auxdata').hide();
    modal.find('#resdata').hide();
  }

  function loadItem (modal, itemData) {
    const item = itemData.item;
    if (item) {
      reset(modal);
      modal.find('#editObjectType').val(item.obj_type);
      modal.find('#editObjectType').prop('disabled', true);
      modal.find('#editObjectType').prop('readonly', true);
      modal.find('#editdObjectIdTxt').text(item.object_id).closest('.form-group').show();
      modal.find('#objectId').val(item.object_id).prop('disabled', true);

      const metatable = modal.find('#metadata');
      const detailButton = modal.find('#detailbutton');
      let tbody;
      if (item.metadata && item.metadata.metadata.length) {
        detailButton.show();
        $('<thead><tr><th colspan="2">Metadata</th></tr></thead>').appendTo(metatable);
        tbody = $('<tbody></tbody>');
        for (let i = 0; i < item.metadata.metadata.length; i++) {
          appendMetaItem(tbody, item.metadata.metadata[i].metaname, item.metadata.metadata[i].metavalue)
        }
        metatable.append(tbody);
      }

      const auxtable = modal.find('#auxdata');
      if (item.auxdata && item.auxdata.auxdata.length) {
        detailButton.show();
        $('<thead><tr><th colspan="2">Aux Data</th></tr></thead>').appendTo(auxtable);
        tbody = $('<tbody></tbody>');
        for (let i = 0; i < item.auxdata.auxdata.length; i++) {
          appendMetaItem(tbody, item.auxdata.auxdata[i].auxname, item.auxdata.auxdata[i].auxvalue)
        }
        auxtable.append(tbody);
      }

      const restable = modal.find('#resdata');
      if (item.resources && item.resources.resources.length) {
        detailButton.show();
        for (let i = 0; i < item.resources.resources.length; i++) {
          if (item.resources.resources[i].resname === '----RESOURCE----') {
            $(`<thead><tr><th>Resource</th><th>${item.resources.resources[i].resvalue}</th></tr></thead>`).appendTo(restable);
            tbody = $('<tbody></tbody>');
            restable.append(tbody);
          }
          else {
            appendMetaItem(tbody, item.resources.resources[i].resname, item.resources.resources[i].resvalue)
          }
        }
      }

      switch (item.obj_type) {
        case 'item': {
          loadSimpleItem(modal, item);
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

  function loadSimpleItem (modal, item) {
    modal.find('#editTitle')
      .val(item.title.value)
      .prop('disabled', !item.title.editable)
      .closest('.form-group').show();

    modal.find('#editLocation')
      .val(item.location.value)
      .prop('disabled', !item.location.editable)
      .closest('.form-group').show();

    modal.find('#editClass')
      .val(item.class.value)
      .prop('disabled', true)
      .closest('.form-group').show();

    modal.find('#editDesc')
      .val(item.description.value)
      .prop('disabled', !item.description.editable)
      .closest('.form-group').show();

    modal.find('#editMime')
      .val(item['mime-type'].value)
      .prop('disabled', true)
      .closest('.form-group').show();

    hideFields([
      modal.find('#editProtocol')
    ]);
  }

  function loadContainer (modal, item) {
    modal.find('#editTitle')
      .val(item.title.value)
      .prop('disabled', !item.title.editable)
      .closest('.form-group').show();

    modal.find('#editClass')
      .val(item.class.value)
      .prop('disabled', true)
      .closest('.form-group').show();

    hideFields([
      modal.find('#editLocation'),
      modal.find('#editMime'),
      modal.find('#editDesc'),
      modal.find('#editProtocol')
    ]);
  }

  function loadExternalUrl (modal, item) {
    modal.find('#editTitle')
      .val(item.title.value)
      .prop('disabled', !item.title.editable)
      .closest('.form-group').show();

    modal.find('#editLocation')
      .val(item.location.value)
      .prop('disabled', !item.location.editable)
      .closest('.form-group').show();

    modal.find('#editClass')
      .val(item.class.value)
      .prop('disabled', true)
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
  }

  function saveItem (modal) {
    let item;
    const objectId = modal.find('#objectId');
    const editObjectType = modal.find('#editObjectType');
    const editTitle = modal.find('#editTitle');
    const editLocation = modal.find('#editLocation');
    const editDesc = modal.find('#editDesc');
    const editMime = modal.find('#editMime');
    const editProtocol = modal.find('#editProtocol');

    switch (editObjectType.val()) {
      case 'item': {
        item = {
          object_id: objectId.val(),
          title: editTitle.val(),
          description: editDesc.val()
        };
        break;
      }
      case 'container': {
        item = {
          object_id: objectId.val(),
          title: editTitle.val()
        };
        break;
      }
      case 'external_url': {
        item = {
          object_id: objectId.val(),
          title: editTitle.val(),
          description: editDesc.val(),
          location: editLocation.val(),
          'mime-type': editMime.val(),
          protocol: editProtocol.val()
        };
        break;
      }
    }
    return item;
  }

  function addObject (modal) {
    let item;
    const parentId = modal.find('#addParentId');
    const editObjectType = modal.find('#editObjectType');
    const editClass = modal.find('#editClass');
    const editTitle = modal.find('#editTitle');
    const editLocation = modal.find('#editLocation');
    const editDesc = modal.find('#editDesc');
    const editMime = modal.find('#editMime');
    const editProtocol = modal.find('#editProtocol');

    switch (editObjectType.val()) {
      case 'item': {
        item = {
          parent_id: parentId.val(),
          obj_type: editObjectType.val(),
          class: editClass.val(),
          title: editTitle.val(),
          location: editLocation.val(),
          description: editDesc.val()
        };
        break;
      }
      case 'container': {
        item = {
          parent_id: parentId.val(),
          obj_type: editObjectType.val(),
          class: editClass.val(),
          title: editTitle.val()
        };
        break;
      }
      case 'external_url': {
        item = {
          parent_id: parentId.val(),
          obj_type: editObjectType.val(),
          class: editClass.val(),
          title: editTitle.val(),
          description: editDesc.val(),
          location: editLocation.val(),
          'mime-type': editMime.val(),
          protocol: editProtocol.val()
        };
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
      return loadItem($(this._element), itemData);
    },
    saveItem: function () {
      return saveItem($(this._element));
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
