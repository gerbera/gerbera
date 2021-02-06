/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.items.js - this file is part of Gerbera.

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
$.widget('grb.dataitems', {

  _create: function () {
    this.element.html('');
    this.element.addClass('grb-dataitems');
    const table = $('<div class="d-flex justify-content-start flex-column h-100 bg-light"></div>');
    const data = this.options.data;
    const onDelete = this.options.onDelete;
    const onEdit = this.options.onEdit;
    const onDownload = this.options.onDownload;
    const onAdd = this.options.onAdd;
    const itemType = this.options.itemType;
    const pager = this.options.pager;
    let row, content, text;

    if (data.length > 0) {
      for (let i = 0; i < data.length; i++) {
        const item = data[i];
        content = $('<div class="d-flex align-items-center border-bottom p-1 bg-white" />');

        if (item.image) {
          const img = $('<img class="float-left" src="" style="object-fit: contain" width="36px" height="36px"/>');
          img.attr('src', item.image);
          img.appendTo(content)
        }

        if (item.url) {
          text = $('<a class="flex-grow-1 ml-2 grb-item-url"></a>');
          text.attr('href', item.url).text(item.text).appendTo(content);
        } else {
          text = $('<span class="flex-grow-1 grb-item-url"></span>');
          text.text(item.text).appendTo(content);
        }

        let buttons;
        if (itemType === 'db') {
          buttons = $('<div class="grb-item-buttons"></div>');

          const downloadIcon = $('<a class="btn grb-item-download fa fa-download pl-0" title="Download Item"></a>');
          downloadIcon.appendTo(buttons);
          if (onDownload) {
            downloadIcon.click(item, onDownload);
          }

          const editIcon = $('<a class="btn grb-item-edit fa fa-pencil pl-0" title="Edit Item"></a>');
          editIcon.appendTo(buttons);
          if (onEdit) {
            editIcon.click(item, onEdit);
          }

          const deleteIcon = $('<a class="btn grb-item-edit fa fa-trash-o pl-0" title="Delete Item"></a>');
          deleteIcon.appendTo(buttons);
          if (onDelete) {
            deleteIcon.click(item, function (event) {
              row.remove();
              onDelete(event);
            });
          }
          buttons.appendTo(content);
        } else if (itemType === 'fs') {
          buttons = $('<div></div>');
          buttons.addClass('grb-item-buttons pull-right');

          const addIcon = $('<a class="btn grb-item-add fa fa-plus pl-0" title="Delete Item"></a>');
          addIcon.appendTo(buttons);
          if (onAdd) {
            addIcon.click(item, onAdd);
          }
          buttons.appendTo(content);
        }

        content.addClass('grb-item');
        table.append(content);
      }
    } else {
      content = $('<td></td>');
      $('<span>No Items found</span>').appendTo(content);
      table.append(content);
    }

    const tfoot = this.buildPager(pager);
    table.append(tfoot);

    this.element.append(table);
    this.element.addClass('with-data');
  },

  buildPager: function (pager) {
    const outer = $('<div/>');

    if (pager && pager.pageCount) {
      let maxPages = Math.ceil(pager.totalMatches / pager.itemsPerPage);

      // No point to have a pager on a single page!
      if (maxPages < 2) {
        return outer;
      }

      const grbPager = $('<nav class="grb-pager"></nav>');
      const list = $('<ul class="pagination"></ul>');
      const previous = $('<li class="page-item">' +
          '<a class="page-link" href="#" aria-label="Previous">' +
          '<span aria-hidden="true">&laquo;</span>' +
          '<span class="sr-only">Previous</span></a>' +
          '</li>');
      const next = $('<li class="page-item">' +
          '<a class="page-link" href="#" aria-label="Next">' +
          '<span aria-hidden="true">&raquo;</span>' +
          '<span class="sr-only">Next</span></a>' +
          '</li>');
      let pageItem;
      let pageLink;
      let pageParams;

      list.append(previous);

      if (pager.onNext) {
        pageParams = {
          itemsPerPage: pager.itemsPerPage,
          totalMatches: pager.totalMatches,
          parentId: pager.parentId
        };
        next.find('a').click(pageParams, pager.onNext);
      }

      if (pager.onPrevious) {
        pageParams = {
          itemsPerPage: pager.itemsPerPage,
          totalMatches: pager.totalMatches,
          parentId: pager.parentId
        };
        previous.find('a').click(pageParams, pager.onPrevious);
      }

      for (let page = 1; page <= maxPages; page++) {
        pageItem = $('<li class="page-item"></li>');
        pageLink = $('<a class="page-link" href="#">' + page + '</a>');
        pageLink.appendTo(pageItem);

        if (pager.onClick) {
          pageParams = {
            pageNumber: page,
            itemsPerPage: pager.itemsPerPage,
            parentId: pager.parentId
          };
          pageLink.click(pageParams, pager.onClick);
        }

        if (page === pager.currentPage) {
          pageItem.addClass('active');
        }

        list.append(pageItem);
      }
      list.append(next);
      grbPager.append(list);
      outer.append(grbPager);
      outer.addClass("py-2 d-flex flex-fill justify-content-center align-items-end");
    }

    return outer;
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-dataitems');
    this.element.removeClass('with-data');
  }

});
