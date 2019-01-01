/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.trail.js - this file is part of Gerbera.

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
/* global $ */

$.widget('grb.trail', {

  _create: function () {
    this.element.addClass('grb-trail')
    var data = this.options.data
    if (data.length > 0) {
      this.render(data, this.options.config)
    }
  },

  _destroy: function () {
    this.element.children('div').remove()
    this.element.removeClass('grb-trail')
  },

  clear: function () {
    this.element.children('div').remove()
  },

  render: function (data, config) {
    this.element.html('')
    this.generate(this.element, data, config)
  },

  generate: function (parent, data, config) {
    var noOp = function () { return false }
    var trailContainer = $('<div></div>')
    var itemBreadcrumb = $('<ol></ol>')
    itemBreadcrumb.addClass('breadcrumb my-auto')

    for (var i = 0; i < data.length; i++) {
      var item = data[i]
      var crumb = $('<li><a></a></li>')
      crumb.addClass('breadcrumb-item')
      crumb.children('a').text(item.text)
      itemBreadcrumb.append(crumb)
    }
    itemBreadcrumb.appendTo(trailContainer)

    var buttonContainer = $('<div></div>')
    buttonContainer.addClass('col-md-5 col-sm-5 col-xs-5 my-auto')
    var buttons

    buttons = $('<ol></ol>')
    buttons.addClass('grb-trail-buttons pull-right my-auto')

    if (config.enableAdd && config.onAdd) {
      var addItemIcon = this.generateItemButton(item, {
        title: 'Add Item',
        class: 'grb-trail-add',
        iconClass: 'fa-plus',
        click: config.onAdd ? config.onAdd : noOp
      })
      addItemIcon.appendTo(buttons)
    }

    if (config.enableEdit && config.onEdit) {
      var editItemIcon = this.generateItemButton(item, {
        title: 'Edit Item',
        class: 'grb-trail-edit',
        iconClass: 'fa-pencil',
        click: config.onEdit ? config.onEdit : noOp
      })
      editItemIcon.appendTo(buttons)
    }

    var autoscanItemIcon
    if (config.enableAddAutoscan && config.onAddAutoscan) {
      autoscanItemIcon = this.generateItemButton(item, {
        title: 'Add Autoscan Item',
        class: 'grb-trail-add-autoscan',
        iconClass: 'fa-history',
        click: config.onAddAutoscan ? config.onAddAutoscan : noOp
      })
      autoscanItemIcon.appendTo(buttons)
    } else if (config.enableEditAutoscan && config.onEditAutoscan) {
      autoscanItemIcon = this.generateItemButton(item, {
        title: 'Edit Autoscan Item',
        class: 'grb-trail-edit-autoscan',
        iconClass: 'fa-history',
        click: config.onEditAutoscan ? config.onEditAutoscan : noOp
      })
      autoscanItemIcon.appendTo(buttons)
    }

    if (config.enableDelete && config.onDelete) {
      var deleteItemIcon = this.generateItemButton(item, {
        title: 'Delete Item',
        class: 'grb-trail-delete',
        iconClass: 'fa-trash-o',
        click: config.onDelete ? config.onDelete : noOp
      })
      deleteItemIcon.appendTo(buttons)
    }

    if (config.enableDeleteAll && config.onDeleteAll) {
      var deleteAllIcon = this.generateItemButton(item, {
        title: 'Delete All',
        class: 'grb-trail-delete-all',
        iconClass: 'fa-remove',
        click: config.onDeleteAll ? config.onDeleteAll : noOp
      })
      deleteAllIcon.appendTo(buttons)
    }

    buttons.appendTo(buttonContainer)
    trailContainer.addClass('col-md-7 col-sm-7 col-xs-7')

    parent.append(trailContainer)
    parent.append(buttonContainer)
  },

  generateItemButton: function (item, config) {
    var button = $('<li>');
    var link = $('<a>', {"title": config.title, "class": config.class, "href": "javascript:;"});
    var icon = $('<i></i>', {"class": "fa " + config.iconClass })
    link.append(icon)
    link.append(" " + config.title)
    button.append(link)
    link.click(item, config.click)
    return button
  },

  length: function () {
    return this.element.find('.breadcrumb li').length
  },

  items: function () {
    return this.element.find('li')
  }

})
