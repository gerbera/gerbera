/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.tweak.js - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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
  const loadItem = function (modal, itemData) {
    const allValues = itemData.values.item;
    const dirTweakLocation = modal.find('#dirTweakLocation');
    const dirTweakId = modal.find('#dirTweakId');
    const dirTweakIndex = modal.find('#dirTweakIndex');
    const dirTweakStatus = modal.find('#dirTweakStatus');
    const dirTweakInherit = modal.find('#dirTweakInherit');
    const dirTweakSymLinks = modal.find('#dirTweakSymLinks');
    const dirTweakRecursive = modal.find('#dirTweakRecursive');
    const dirTweakCaseSens = modal.find('#dirTweakCaseSens');
    const dirTweakHidden = modal.find('#dirTweakHidden');
    const dirTweakMetaCharset = modal.find('#dirTweakMetaCharset');
    const dirTweakFanArt = modal.find('#dirTweakFanArt');
    const dirTweakSubtitle = modal.find('#dirTweakSubtitle');
    const dirTweakResource = modal.find('#dirTweakResource');
    const dirTweakDelete = modal.find('#dirTweakDelete');
    const dirTweakSave = modal.find('#dirTweakSave');

    let item = null;
    const xPath = "/import/directories/tweak";
    let tweakList = {};
    let tweakMap = {};
    let itemCount = 0;
    if (allValues) {
      allValues.forEach((val) => {
        if (val.item.startsWith(xPath)) {
          var entry = val.item.replace(xPath, '');
          const re = /^\[([0-9]+)\]+.*/;
          const result = re.exec(entry);
          if (result) {
            const myIndex = Number.parseInt(result[1]);
            const key = entry.replace(`[${myIndex}]/attribute::`, '');
            
            if (!(myIndex in tweakList)) {
              tweakList[myIndex] = {id: val.id, index: myIndex};
                itemCount = itemCount>myIndex ? itemCount : myIndex;
            }
            if (key != `[${myIndex}]`) {
              tweakList[myIndex][key] = val.value;
              if (key === 'location') {
                tweakMap[val.value] = myIndex;
              }
            } else {
              tweakList[myIndex].status = val.status
            }
          }
        }
      });
    }
    reset(modal);

    dirTweakLocation.val(itemData.path);
    
    if (itemData.path in tweakMap) {
      item = tweakList[tweakMap[itemData.path]];
      dirTweakDelete.prop('hidden', false);
      dirTweakDelete.prop('disabled', false);
      dirTweakDelete.off('click').on('click', itemData.onDelete);
    } else {
      item = {
        id: -1,
        status: 'added',
        index: itemCount
      };
    }

    if (item) {
      dirTweakSave.prop('disabled', false);
      dirTweakSave.off('click').on('click', itemData.onSave);
      dirTweakId.val(item.id);
      dirTweakIndex.val(item.index);
      dirTweakStatus.val(item.status);

      dirTweakInherit.prop('checked', item.inherit === 'true');
      dirTweakSymLinks.prop('checked', item['follow-symlinks'] === 'true');
      dirTweakRecursive.prop('checked', item.recursive === 'true');
      dirTweakHidden.prop('checked', item['hidden-files'] === 'true');
      dirTweakCaseSens.prop('checked', item['case-sensitive'] === 'true');
      dirTweakMetaCharset.val(item['meta-charset']);
      dirTweakFanArt.val(item['fanart-file']);
      dirTweakResource.val(item['subtitle-file']);
      dirTweakSubtitle.val(item['resource-file']);
    }
  };

  const reset = function (modal) {
    const dirTweakLocation = modal.find('#dirTweakLocation');
    const dirTweakId = modal.find('#dirTweakId');
    const dirTweakIndex = modal.find('#dirTweakIndex');
    const dirTweakStatus = modal.find('#dirTweakStatus');
    const dirTweakInherit = modal.find('#dirTweakInherit');
    const dirTweakSymLinks = modal.find('#dirTweakSymLinks');
    const dirTweakRecursive = modal.find('#dirTweakRecursive');
    const dirTweakCaseSens = modal.find('#dirTweakCaseSens');
    const dirTweakHidden = modal.find('#dirTweakHidden');
    const dirTweakMetaCharset = modal.find('#dirTweakMetaCharset');
    const dirTweakFanArt = modal.find('#dirTweakFanArt');
    const dirTweakSubtitle = modal.find('#dirTweakSubtitle');
    const dirTweakResource = modal.find('#dirTweakResource');
    const dirTweakDelete = modal.find('#dirTweakDelete');
    const dirTweakSave = modal.find('#dirTweakSave');

    modal.find('form :input').each(function () {
      $(this).prop('disabled', false);
    });

    dirTweakLocation.val('/tmp');
    dirTweakId.val(-1);
    dirTweakIndex.val(0);
    dirTweakStatus.val('');
    dirTweakLocation.prop('disabled', true);
    dirTweakInherit.prop('checked', false);
    dirTweakSymLinks.prop('checked', false);
    dirTweakRecursive.prop('checked', false);
    dirTweakCaseSens.prop('checked', false);
    dirTweakHidden.prop('checked', false);
    dirTweakMetaCharset.val('');
    dirTweakFanArt.val('');
    dirTweakSubtitle.val('');
    dirTweakResource.val('');
    dirTweakDelete.prop('disabled', true);
    dirTweakDelete.prop('hidden', true);
    dirTweakSave.prop('disabled', true);
  };

  function saveItem (modal) {
    const dirTweakLocation = modal.find('#dirTweakLocation');
    const dirTweakId = modal.find('#dirTweakId');
    const dirTweakIndex = modal.find('#dirTweakIndex');
    const dirTweakStatus = modal.find('#dirTweakStatus');
    const dirTweakInherit = modal.find('#dirTweakInherit');
    const dirTweakSymLinks = modal.find('#dirTweakSymLinks');
    const dirTweakRecursive = modal.find('#dirTweakRecursive');
    const dirTweakCaseSens = modal.find('#dirTweakCaseSens');
    const dirTweakHidden = modal.find('#dirTweakHidden');
    const dirTweakMetaCharset = modal.find('#dirTweakMetaCharset');
    const dirTweakFanArt = modal.find('#dirTweakFanArt');
    const dirTweakSubtitle = modal.find('#dirTweakSubtitle');
    const dirTweakResource = modal.find('#dirTweakResource');

    let item = {
      location: dirTweakLocation.val(),
      id: dirTweakId.val(),
      index: dirTweakIndex.val(),
      status: dirTweakStatus.val(),
      inherit: dirTweakInherit.is(':checked'),
      'follow-symlinks': dirTweakSymLinks.is(':checked'),
      recursive: dirTweakRecursive.is(':checked'),
      'case-sensitive': dirTweakCaseSens.is(':checked'),
      'hidden-files': dirTweakHidden.is(':checked'),
      'meta-charset': dirTweakMetaCharset.val(),
      'fanart-file': dirTweakFanArt.val(),
      'subtitle-file': dirTweakSubtitle.val(),
      'resource-file': dirTweakResource.val(),
    };
    return item;
  }

  function deleteItem (modal) {
    const dirTweakLocation = modal.find('#dirTweakLocation');
    const dirTweakId = modal.find('#dirTweakId');
    const dirTweakIndex = modal.find('#dirTweakIndex');
    const dirTweakStatus = modal.find('#dirTweakStatus');
    const dirTweakInherit = modal.find('#dirTweakInherit');
    const dirTweakSymLinks = modal.find('#dirTweakSymLinks');
    const dirTweakRecursive = modal.find('#dirTweakRecursive');
    const dirTweakCaseSens = modal.find('#dirTweakCaseSens');
    const dirTweakHidden = modal.find('#dirTweakHidden');
    const dirTweakMetaCharset = modal.find('#dirTweakMetaCharset');
    const dirTweakFanArt = modal.find('#dirTweakFanArt');
    const dirTweakSubtitle = modal.find('#dirTweakSubtitle');
    const dirTweakResource = modal.find('#dirTweakResource');

    let item = {
      location: dirTweakLocation.val(),
      id: dirTweakId.val(),
      index: dirTweakIndex.val(),
      status: dirTweakStatus.val() === 'manual' ? 'killed' : 'reset',
      inherit: dirTweakInherit.is(':checked'),
      'follow-symlinks': dirTweakSymLinks.is(':checked'),
      recursive: dirTweakRecursive.is(':checked'),
      'case-sensitive': dirTweakCaseSens.is(':checked'),
      'hidden-files': dirTweakHidden.is(':checked'),
      'meta-charset': dirTweakMetaCharset.val(),
      'fanart-file': dirTweakFanArt.val(),
      'subtitle-file': dirTweakSubtitle.val(),
      'resource-file': dirTweakResource.val(),
    };
    return item;
  }

  let _super = $.fn.modal;

  // create a new constructor
  let GerberaDirTweakModal = function (element, options) { // eslint-disable-line
    _super.Constructor.apply(this, arguments);
  };

  GerberaDirTweakModal.prototype = $.extend({}, _super.Constructor.prototype, {
    constructor: GerberaDirTweakModal,
    _super: function () {
      let args = $.makeArray(arguments);
      _super.Constructor.prototype[args.shift()].apply(this, args);
    },
    loadItem: function (itemData) {
      return loadItem($(this._element), itemData);
    },
    reset: function () {
      return reset($(this._element));
    },
    saveItem: function () {
      return saveItem($(this._element));
    },
    deleteItem: function () {
      return deleteItem($(this._element));
    }
  });

  // override the old initialization with the new constructor
  $.fn.dirtweakmodal = $.extend(function (option) {
    let args = $.makeArray(arguments);
    let retval = null;
    option = args.shift();

    this.each(function () {
      let $this = $(this);
      let data = $this.data('modal');
      let options = $.extend({}, _super.defaults, $this.data(), typeof option === 'object' && option);

      if (!data) {
        $this.data('modal', (data = new GerberaDirTweakModal(this, options)));
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
  }, $.fn.dirtweakmodal);
})(jQuery);
