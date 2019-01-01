/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.items.js - this file is part of Gerbera.

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
/* global $ GERBERA */

$.widget('grb.dataitems', {

  _create: function () {
    this.element.html('')
    this.element.addClass('grb-dataitems')
    var table = $('<table></table>').addClass('table')
    var tbody = $('<tbody></tbody>')
    var data = this.options.data
    var onDelete = this.options.onDelete
    var onEdit = this.options.onEdit
    var onDownload = this.options.onDownload
    var onAdd = this.options.onAdd
    var itemType = this.options.itemType
    var pager = this.options.pager
    var row, content, text

    if (data.length > 0) {
      for (var i = 0; i < data.length; i++) {
        var item = data[i]
        row = $('<tr></tr>')
        content = $('<td></td>')

        if (item.img) {
          var img = $('<img/>')
          img.attr('src', item.img)
          img.attr('style', 'height: 25px')
          img.addClass('rounded float-left')
          img.appendTo(content)
        }

        if (item.url) {
          text = $('<a></a>')
          text.attr('href', item.url).text(item.text).appendTo(content)
        } else {
          text = $('<span></span>')
          text.text(item.text).appendTo(content)
        }
        text.addClass('grb-item-url')

        if (item.video) {
          var button = $('<button></button>')
          button.text('Play Video')
          button.addClass('pull-right')
          button.appendTo(content)
          button.click({item: item}, GERBERA.Items.playVideo)
        }

        var buttons
        if (itemType === 'db') {
          buttons = $('<div></div>')
          buttons.addClass('grb-item-buttons pull-right')

          var downloadIcon = $('<span></span>')
          downloadIcon.prop('title', 'Download item')
          downloadIcon.addClass('grb-item-download fa fa-download')
          downloadIcon.appendTo(buttons)
          if (onDownload) {
            downloadIcon.click(item, onDownload)
          }

          var editIcon = $('<span></span>')
          editIcon.prop('title', 'Edit item')
          editIcon.addClass('grb-item-edit fa fa-pencil')
          editIcon.appendTo(buttons)
          if (onEdit) {
            editIcon.click(item, onEdit)
          }

          var deleteIcon = $('<span></span>')
          deleteIcon.prop('title', 'Delete item')
          deleteIcon.addClass('grb-item-delete fa fa-trash-o')
          deleteIcon.appendTo(buttons)
          if (onDelete) {
            deleteIcon.click(item, function (event) {
              row.remove();
              onDelete(event);
            })
          }
          buttons.appendTo(content)
        } else if (itemType === 'fs') {
          buttons = $('<div></div>')
          buttons.addClass('grb-item-buttons pull-right')

          var addIcon = $('<span></span>')
          addIcon.prop('title', 'Add item')
          addIcon.addClass('grb-item-add fa fa-plus')
          addIcon.appendTo(buttons)
          if (onAdd) {
            addIcon.click(item, onAdd)
          }
          buttons.appendTo(content)
        }

        row.addClass('grb-item')
        row.append(content)
        tbody.append(row)
      }
    } else {
      row = $('<tr></tr>')
      content = $('<td></td>')
      text = $('<span>No Items found</span>').appendTo(content)
      row.append(content)
      tbody.append(row)
    }
    tbody.appendTo(table)

    var tfoot = this.buildFooter(pager)
    table.append(tfoot)

    this.element.append(table)
    this.element.addClass('with-data')
  },

  buildFooter: function (pager) {
    var tfoot = $('<tfoot><tr><td></td></tr></tfoot>')
    var grbPager = $('<nav class="grb-pager"></nav>')
    var list = $('<ul class="pagination"></ul>')
    var previous = $('<li class="page-item">' +
        '<a class="page-link" href="#" aria-label="Previous">' +
        '<span aria-hidden="true">&laquo;</span>' +
        '<span class="sr-only">Previous</span></a>' +
        '</li>')
    var next = $('<li class="page-item">' +
      '<a class="page-link" href="#" aria-label="Next">' +
      '<span aria-hidden="true">&raquo;</span>' +
      '<span class="sr-only">Next</span></a>' +
      '</li>')
    var maxPages
    var pageItem
    var pageLink
    var pageParams

    list.append(previous)

    if (pager && pager.pageCount) {
      maxPages = Math.ceil(pager.totalMatches / pager.itemsPerPage)

      if (pager.onNext) {
        pageParams = {
          itemsPerPage: pager.itemsPerPage,
          totalMatches: pager.totalMatches,
          parentId: pager.parentId
        }
        next.find('a').click(pageParams, pager.onNext)
      }

      if (pager.onPrevious) {
        pageParams = {
          itemsPerPage: pager.itemsPerPage,
          totalMatches: pager.totalMatches,
          parentId: pager.parentId
        }
        previous.find('a').click(pageParams, pager.onPrevious)
      }

      for (var page = 1; page <= maxPages; page++) {
        pageItem = $('<li class="page-item"></li>')
        pageLink = $('<a class="page-link" href="#">' + page + '</a>')
        pageLink.appendTo(pageItem)

        if (pager.onClick) {
          pageParams = {
            pageNumber: page,
            itemsPerPage: pager.itemsPerPage,
            parentId: pager.parentId
          }
          pageLink.click(pageParams, pager.onClick)
        }

        if (page === pager.currentPage) {
          pageItem.addClass('active')
        }

        list.append(pageItem)
      }
      list.append(next)
      grbPager.append(list)
      tfoot.find('td').append(grbPager)
    }

    return tfoot
  },

  _destroy: function () {
    this.element.children('table').remove()
    this.element.removeClass('grb-dataitems')
    this.element.removeClass('with-data')
  }

})
