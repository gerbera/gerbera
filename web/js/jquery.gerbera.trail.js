/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.trail.js - this file is part of Gerbera.

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
$.widget('grb.trail', {

  _create: function () {
    this.element.addClass('grb-trail');
    const data = this.options.data;
    if (data.length > 0) {
      this.render(data, this.options.config);
    }
  },

  _destroy: function () {
    this.element.children('div').remove();
    this.element.removeClass('grb-trail');
  },

  clear: function () {
    this.element.children('div').remove();
  },

  render: function (data, config) {
    this.element.html('');
    this.generate(this.element, data, config);
  },

  generate: function (parent, data, config) {
    const noOp = function () { return false };
    const trailContainer = $('<div></div>');
    const itemBreadcrumb = $('<ol></ol>');
    itemBreadcrumb.addClass('breadcrumb my-auto');

    let item;
    for (let i = 0; i < data.length; i++) {
      item = data[i];
      const crumb = $('<li><a></a></li>');
      crumb.addClass('breadcrumb-item');
      crumb.children('a').text(item.text);
      itemBreadcrumb.append(crumb);
    }
    itemBreadcrumb.appendTo(trailContainer);

    const buttonContainer = $('<div></div>');
    buttonContainer.addClass('col-4 col-sm-5 my-auto');
    let buttons;

    buttons = $('<ol></ol>');
    buttons.addClass('grb-trail-buttons pull-right my-auto');

    if (config.enableAdd && config.onAdd) {
      if (config.itemType === 'fs') {
        // We're in filesystem view
        const addItemIcon = this.generateItemButton(item, {
          title: 'Add Items',
          tooltip: 'Add folder and subfolders to database',
          class: 'grb-trail-add',
          iconClass: 'fa-plus',
          click: config.onAdd ? config.onAdd : noOp
        });
        addItemIcon.appendTo(buttons);
      } else {
        // We're in database view
        const addItemIcon = this.generateItemButton(item, {
          title: 'Add Item',
          tooltip: 'Add Container or Item to virtual layout',
          class: 'grb-trail-add',
          iconClass: 'fa-plus',
          click: config.onAdd ? config.onAdd : noOp
        });
        addItemIcon.appendTo(buttons);
      }
    }

    if (config.enableEdit && config.onEdit) {
      const editItemIcon = this.generateItemButton(item, {
        title: 'Edit Item',
        class: 'grb-trail-edit',
        iconClass: 'fa-pencil',
        click: config.onEdit ? config.onEdit : noOp
      });
      editItemIcon.appendTo(buttons);
    }

    if (config.enableRunScan && config.onRunScan) {
      const autoscanItemIcon = this.generateItemButton(item, {
        title: 'Scan Now',
        tooltip: 'Run Autoscan Immediately',
        class: 'grb-trail-run-autoscan',
        iconClass: 'fa-spinner',
        click: config.onRunScan ? config.onRunScan : noOp
      });
      autoscanItemIcon.appendTo(buttons);
    }
    if (config.enableAddAutoscan && config.onAddAutoscan) {
      const autoscanItemIcon = this.generateItemButton(item, {
        title: 'Add Autoscan Directory',
        tooltip: 'Configure folder as autoscan item',
        class: 'grb-trail-add-autoscan',
        iconClass: 'fa-history',
        click: config.onAddAutoscan ? config.onAddAutoscan : noOp
      });
      autoscanItemIcon.appendTo(buttons);
    } else if (config.enableEditAutoscan && config.onEditAutoscan) {
      const autoscanItemIcon = this.generateItemButton(item, {
        title: 'Edit Autoscan Item',
        class: 'grb-trail-edit-autoscan',
        iconClass: 'fa-history',
        click: config.onEditAutoscan ? config.onEditAutoscan : noOp
      });
      autoscanItemIcon.appendTo(buttons);
    }

    if (config.enableDelete && config.onDelete) {
      const deleteItemIcon = this.generateItemButton(item, {
        title: 'Delete Container',
        tooltip: 'Delete container and items of container',
        class: 'grb-trail-delete',
        iconClass: 'fa-trash-o',
        click: config.onDelete ? config.onDelete : noOp
      });
      deleteItemIcon.appendTo(buttons);
    }

    if (config.enableDeleteAll && config.onDeleteAll) {
      const deleteAllIcon = this.generateItemButton(item, {
        title: 'Delete with References',
        tooltip: 'Delete container, items and all references',
        class: 'grb-trail-delete-all',
        iconClass: 'fa-remove',
        click: config.onDeleteAll ? config.onDeleteAll : noOp
      });
      deleteAllIcon.appendTo(buttons);
    }

    if (config.enableClearConfig && config.onClear) {
      const clearIcon = this.generateItemButton(item, {
        title: 'Clear Saved Config',
        class: 'grb-trail-clear-config',
        iconClass: 'fa-trash',
        click: config.onClear ? config.onClear : noOp
      });

      clearIcon.appendTo(buttons);
    }
    if (config.enableConfig && config.onSave && config.onRescan) {
      const saveIcon = this.generateItemButton(item, {
        title: 'Save Config',
        class: 'grb-trail-save-config',
        iconClass: 'fa-save',
        click: config.onSave ? config.onSave : noOp
      });
      const rescanIcon = this.generateItemButton(item, {
        title: 'Rescan Library',
        class: 'grb-trail-rescan',
        iconClass: 'fa-retweet',
        click: config.onRescan ? config.onRescan : noOp
      });

      rescanIcon.appendTo(buttons);
      saveIcon.appendTo(buttons);
    }

    if (config.enableAddTweak && config.onAddTweak) {
      const tweakIcon = this.generateItemButton(item, {
        title: 'Tweak Directory',
        tooltip: 'Set special import properties for folder',
        class: 'grb-trail-tweak-dir',
        iconClass: 'fa-address-card-o',
        click: config.onAddTweak ? config.onAddTweak : noOp
      });
      tweakIcon.appendTo(buttons);
    }

    buttons.appendTo(buttonContainer);
    trailContainer.addClass('col-8 col-sm-7');

    parent.append(trailContainer);
    parent.append(buttonContainer);
  },

  generateItemButton: function (item, config) {
    const button = $('<li>');
    const link = $('<a>', { "title": config.tooltip ? config.tooltip : config.title, "class": "grb-trail-button " + config.class, "href": "javascript:;" });
    const icon = $('<i></i>', { "class": "fa " + config.iconClass });
    link.append(icon);
    link.append('<span> ' + config.title + '</span>');
    button.append(link);
    link.click(item, config.click);
    return button
  },

  length: function () {
    return this.element.find('.breadcrumb li').length;
  },

  items: function () {
    return this.element.find('li');
  }

});
