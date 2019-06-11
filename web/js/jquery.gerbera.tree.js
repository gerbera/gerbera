/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.tree.js - this file is part of Gerbera.

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

$.widget('grb.tree', {

  _create: function () {
    this.element.addClass('grb-tree');
    const data = this.options.data;
    if (data.length > 0) {
      this.render(data, this.options.config);
    }
  },

  _destroy: function () {
    this.element.children('ul').remove();
    this.element.removeClass('grb-tree');
  },

  clear: function () {
    this.element.children('ul').remove();
  },

  render: function (data, config) {
    this.element.html('');
    this.generate(this.element, data, config);
  },

  generate: function (parent, data, config) {
    const items = [];
    const newList = $('<ul></ul>');
    newList.addClass('list-group');

    for (let i = 0; i < data.length; i++) {
      const item = $('<li></li>');
      item.addClass('list-group-item');
      item.data('grb-id', data[i].gerbera.id);
      item.data('grb-item', data[i]);
      item.prop('id', 'grb-tree-' + data[i].gerbera.id);

      const icon = $('<span></span>').addClass(config.closedIcon).addClass('folder-icon');
      const title = $('<span></span>').addClass(config.titleClass).text(data[i].title);
      if (config.onSelection) {
        title.click(data[i], config.onSelection);
      }
      if (config.onExpand) {
        title.click(data[i], config.onExpand);
      }

      const badges = [];
      const badgeData = data[i].badge;
      if (badgeData && badgeData.length > 0) {
        for (let x = 0; x < badgeData.length; x++) {
          const aBadge = $('<span></span>').addClass('badge badge-pill badge-secondary').text(badgeData[x]);
          aBadge.addClass('pull-right');

          if (badgeData[x] === 'a') {
            aBadge.addClass('autoscan');
            aBadge.click({id: data[i].gerbera.id}, config.onAutoscanEdit);
            aBadge.prop('title', 'Autoscan: ' + data[i].gerbera.autoScanType);
          } else if (!isNaN(badgeData[x])) {
            aBadge.prop('title', badgeData[x] + ' - children');
          }

          badges.push(aBadge);
        }
      }

      item.append(icon);
      item.append(title);
      item.append(badges);
      items.push(item);
      if (data[i].nodes && data[i].nodes.length > 0) {
        item.children('span').first().removeClass(config.closedIcon).addClass(config.openIcon);
        this.generate(item, data[i].nodes, config);
      }
    }
    newList.append(items);
    parent.append(newList);
  },

  append: function (parent, data) {
    parent.children('span').first().removeClass(this.options.config.closedIcon).addClass(this.options.config.openIcon);
    this.generate(parent, data, this.options.config);
  },

  closed: function (element) {
    return $(element).prev('span.folder-icon').hasClass(this.options.config.closedIcon);
  },

  collapse: function (element) {
    element.children('span').first().removeClass(this.options.config.openIcon).addClass(this.options.config.closedIcon);
    element.find('ul.list-group').remove();
  },

  select: function (folderList) {
    this.element.find('li').removeClass('selected-item');
    $(folderList).addClass('selected-item');
  },

  getItem: function (id) {
    let gerberaItem;
    const treeElement = this.getElement(id);
    if (treeElement.length === 1) {
      gerberaItem = treeElement.data('grb-item');
    } else {
      gerberaItem = false;
    }
    return gerberaItem;
  },

  getElement: function (id) {
    return this.element.find('#grb-tree-' + id);
  }

});
