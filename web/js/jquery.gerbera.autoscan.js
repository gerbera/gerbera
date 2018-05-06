/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.autoscan.js - this file is part of Gerbera.

    Copyright (C) 2016-2018 Gerbera Contributors

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
  var loadItem = function (modal, itemData) {
    var item = itemData.item
    var autoscanId = modal.find('#autoscanId')
    var autoscanIdTxt = modal.find('#autoscanIdTxt')
    var autoscanFromFs = modal.find('#autoscanFromFs')
    var autoscanMode = modal.find('input[name=autoscanMode]')
    var autoscanLevel = modal.find('input[name=autoscanLevel]')
    var autoscanPersistent = modal.find('#autoscanPersistent')
    var autoscanRecursive = modal.find('#autoscanRecursive')
    var autoscanHidden = modal.find('#autoscanHidden')
    var autoscanInterval = modal.find('#autoscanInterval')
    var autoscanSave = modal.find('#autoscanSave')
    var autoscanPersistentMsg = modal.find('#autoscan-persistent-msg')

    reset(modal)
    if (item) {
      if (item.persistent) {
        modal.find('form :input').each(function () {
          $(this).prop('disabled', true)
        })
        autoscanSave.prop('disabled', true).off('click')
        autoscanPersistentMsg.show()
      } else {
        autoscanSave.off('click').on('click', itemData.onSave)
      }

      autoscanId.val(item.object_id)
      autoscanIdTxt.text(item.object_id)
      autoscanIdTxt.prop('title', item.object_id)
      autoscanFromFs.prop('checked', item.from_fs)
      autoscanMode.val([item.scan_mode])
      autoscanLevel.val([item.scan_level])
      autoscanPersistent.prop('checked', item.persistent)
      autoscanRecursive.prop('checked', item.recursive)
      autoscanHidden.prop('checked', item.hidden)
      autoscanInterval.val(item.interval)

      autoscanMode.off('click').on('click', function () {
        adjustFieldApplicability(modal)
      })

      adjustFieldApplicability(modal)
    }
  }

  var adjustFieldApplicability = function (modal) {
    var autoscanMode = modal.find('input[name=autoscanMode]:checked')
    var autoscanLevel = modal.find('input[name=autoscanLevel]')
    var autoscanRecursive = modal.find('#autoscanRecursive')
    var autoscanHidden = modal.find('#autoscanHidden')
    var autoscanInterval = modal.find('#autoscanInterval')
    var autoscanPersistent = modal.find('#autoscanPersistent')

    if (autoscanPersistent.is(':checked')) {
      modal.find('form :input').each(function () {
        $(this).prop('disabled', true)
      })
    } else {
      switch (autoscanMode.val()) {
        case 'timed':
          autoscanLevel.closest('.form-check').removeClass('disabled').show()
          autoscanLevel.prop('disabled', false)
          autoscanRecursive.closest('.form-check').removeClass('disabled').show()
          autoscanRecursive.prop('disabled', false)
          autoscanHidden.closest('.form-check').removeClass('disabled').show()
          autoscanHidden.prop('disabled', false)
          autoscanInterval.closest('.form-group').removeClass('disabled').show()
          autoscanInterval.prop('disabled', false)
          break
        case 'inotify':
          autoscanLevel.closest('.form-check').removeClass('disabled').show()
          autoscanLevel.prop('disabled', false)
          autoscanRecursive.closest('.form-check').removeClass('disabled').show()
          autoscanRecursive.prop('disabled', false)
          autoscanHidden.closest('.form-check').removeClass('disabled').show()
          autoscanHidden.prop('disabled', false)
          autoscanInterval.closest('.form-group').hide()
          autoscanInterval.prop('disabled', true)
          break
        case 'none':
          autoscanLevel.closest('.form-check').addClass('disabled').show()
          autoscanLevel.prop('disabled', true)
          autoscanRecursive.closest('.form-check').addClass('disabled').show()
          autoscanRecursive.prop('disabled', true)
          autoscanHidden.closest('.form-check').addClass('disabled').show()
          autoscanHidden.prop('disabled', true)
          autoscanInterval.closest('.form-group').addClass('disabled').show()
          autoscanInterval.prop('disabled', true)
          break
      }
    }
  }

  var reset = function (modal) {
    var autoscanId = modal.find('#autoscanId')
    var autoscanIdTxt = modal.find('#autoscanIdTxt')
    var autoscanFromFs = modal.find('#autoscanFromFs')
    var autoscanMode = modal.find('input[name=autoscanMode]')
    var autoscanLevel = modal.find('input[name=autoscanLevel]')
    var autoscanPersistent = modal.find('#autoscanPersistent')
    var autoscanRecursive = modal.find('#autoscanRecursive')
    var autoscanHidden = modal.find('#autoscanHidden')
    var autoscanInterval = modal.find('#autoscanInterval')
    var autoscanSave = modal.find('#autoscanSave')
    var autoscanPersistentMsg = modal.find('#autoscan-persistent-msg')

    modal.find('form :input').each(function () {
      $(this).prop('disabled', false)
    })

    autoscanId.val('')
    autoscanIdTxt.text('')
    autoscanFromFs.prop('checked', false)
    autoscanMode.val([''])
    autoscanLevel.val([''])
    autoscanPersistent.prop('checked', false)
    autoscanRecursive.prop('checked', false)
    autoscanHidden.prop('checked', false)
    autoscanInterval.val('')
    autoscanSave.prop('disabled', false)
    autoscanPersistentMsg.hide()
  }

  function saveItem (modal) {
    var objectId = modal.find('#autoscanId')
    var fromFs = modal.find('#autoscanFromFs')
    var autoscanMode = modal.find('input:radio[name=autoscanMode]:checked')
    var autoscanLevel = modal.find('input:radio[name=autoscanLevel]:checked')
    var autoscanRecursive = modal.find('#autoscanRecursive')
    var autoscanHidden = modal.find('#autoscanHidden')
    var autoscanInterval = modal.find('#autoscanInterval')

    var item = {
      object_id: objectId.val(),
      from_fs: fromFs.is(':checked'),
      scan_mode: autoscanMode.val()
    }

    switch (autoscanMode.val()) {
      case 'timed':
        item = $.extend({}, item, {
          scan_level: autoscanLevel.val(),
          recursive: autoscanRecursive.is(':checked'),
          hidden: autoscanHidden.is(':checked'),
          interval: autoscanInterval.val()
        })
        break
      case 'inotify':
        item = $.extend({}, item, {
          scan_level: autoscanLevel.val(),
          recursive: autoscanRecursive.is(':checked'),
          hidden: autoscanHidden.is(':checked')
        })
        break
      case 'none':
        break
    }

    return item
  }

  var _super = $.fn.modal

  // create a new constructor
  var GerberaAutoscanModal = function (element, options) { // eslint-disable-line
    _super.Constructor.apply(this, arguments)
  }

  GerberaAutoscanModal.prototype = $.extend({}, _super.Constructor.prototype, {
    constructor: GerberaAutoscanModal,
    _super: function () {
      var args = $.makeArray(arguments)
      _super.Constructor.prototype[args.shift()].apply(this, args)
    },
    loadItem: function (itemData) {
      return loadItem($(this._element), itemData)
    },
    reset: function () {
      return reset($(this._element))
    },
    saveItem: function () {
      return saveItem($(this._element))
    }
  })

  // override the old initialization with the new constructor
  $.fn.autoscanmodal = $.extend(function (option) {
    var args = $.makeArray(arguments)
    var retval = null
    option = args.shift()

    this.each(function () {
      var $this = $(this)
      var data = $this.data('modal')
      var options = $.extend({}, _super.defaults, $this.data(), typeof option === 'object' && option)

      if (!data) {
        $this.data('modal', (data = new GerberaAutoscanModal(this, options)))
      }
      if (typeof option === 'string') {
        // for custom methods return the method value, not the selector
        retval = data[option].apply(data, args)
      } else if (options.show) {
        data.show.apply(data, args)
      }
    })

    if (!retval) {
      retval = this
    }
    return retval
  }, $.fn.autoscanmodal)
})(jQuery)
