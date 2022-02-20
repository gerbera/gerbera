/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.items.js - this file is part of Gerbera.

    Copyright (C) 2016-2022 Gerbera Contributors

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
    const table = $('<table></table>').addClass('table');
    const tbody = $('<tbody></tbody>');
    let tcontainer = tbody;
    const data = this.options.data;
    const onDelete = this.options.onDelete;
    const onEdit = this.options.onEdit;
    const onDownload = this.options.onDownload;
    const onAdd = this.options.onAdd;
    const itemType = this.options.itemType;
    const pager = this.options.pager;
    let row, content, text;

    if (data.length > 0) {
      if (itemType === 'db' && pager && pager.gridMode > 0) {
        row = $('<tr></tr>');
        content = $('<td></td>');
        const div = $('<div style="display: flex; flex-wrap: wrap;"></div>');
        content.append(div);
        row.append(content);
        tbody.append(row);
        tbody.appendTo(table);
        tcontainer = div;
      } else {
        tcontainer.appendTo(table);
      }
      for (let i in data) {
        const item = data[i];
        const gm = (pager) ? pager.gridMode : 0;
        switch (gm) {
          case 1:
            content = $('<div class="grb-item-grid grb-item-grid-normal justify-content-center align-items-center"></div>');
            break;
          case 2:
            content = $('<div class="grb-item-grid grb-item-grid-large justify-content-center align-items-center"></div>');
            break;
          default:
            content = $('<td></td>');
            row = $('<tr class="datagrid-row"></tr>');
        }

        var itemText = item.text;
        if (item.track) {
          itemText = item.track + ' ' + itemText;
        }
        if (item.part) {
          itemText = item.part + ' ' + itemText;
        }

        if (item.url) {
          text = $('<a></a>');
          text.attr('title', 'Open ' + item.text);
          text.attr('type', item.mtype);
          text.attr('href', item.url);
          if (!pager || pager.gridMode === 0) {
            text.text(itemText);
            text.appendTo(content);
          } else {
            const div = $('<div style="display: grid;"></div>');
            text.appendTo(div);
            if (pager.gridMode === 2) {
              const text2 = $('<a></a>');
              text2.attr('title', 'Open ' + item.text);
              text2.attr('type', item.mtype);
              text2.attr('href', item.url);
              text2.text(itemText);
              text2.appendTo(div);
            }
            div.appendTo(content);
          }
        } else {
          text = $('<span></span>');
          text.text(itemText).appendTo(content);
        }
        if (item.image) {
          text.prepend($('<img class="pull-left rounded grb-thumbnail" src="' + item.image + '"/>'));
        } else {
          let icon = "fa-file-o";
          if (item.upnp_class) {
            if (item.upnp_class.startsWith("object.item.audioItem")) {
              icon = "fa-music"
            } else if (item.upnp_class.startsWith("object.item.videoItem")) {
              icon = "fa-film"
            } else if (item.upnp_class.startsWith("object.item.imageItem")) {
              icon = "fa-camera"
            }
          }
          text.prepend($('<div class="d-flex pull-left rounded grb-thumbnail justify-content-center align-items-center"><i class="grb-item-icon text-muted fa ' + icon + '"></i></div>'));
        }
        text.addClass('grb-item-url');

        let buttons;
        if (itemType === 'db') {
          buttons = $('<div></div>');
          buttons.addClass('grb-item-buttons pull-right justify-content-center align-items-center');

          const downloadIcon = $('<span></span>');
          downloadIcon.prop('title', 'Download item');
          downloadIcon.addClass('grb-item-download fa fa-download');
          downloadIcon.appendTo(buttons);
          if (onDownload) {
            downloadIcon.click(item, onDownload);
          }

          if (!pager || pager.gridMode === 0) {
            const editIcon = $('<span></span>');
            editIcon.prop('title', 'Edit item');
            editIcon.addClass('grb-item-edit fa fa-pencil');
            editIcon.appendTo(buttons);
            if (onEdit) {
              editIcon.click(item, onEdit);
            }
            const deleteIcon = $('<span></span>');
            deleteIcon.prop('title', 'Delete item');
            deleteIcon.addClass('grb-item-delete fa fa-trash-o');
            deleteIcon.appendTo(buttons);
            if (onDelete) {
              deleteIcon.click(item, function (event) {
                row.remove();
                onDelete(event);
              });
          }}
          buttons.appendTo(content);
        } else if (itemType === 'fs') {
          buttons = $('<div></div>');
          buttons.addClass('grb-item-buttons pull-right');

          const addIcon = $('<span></span>');
          addIcon.prop('title', 'Add item');
          addIcon.addClass('grb-item-add fa fa-plus');
          addIcon.appendTo(buttons);
          if (onAdd) {
            addIcon.click(item, onAdd);
          }
          buttons.appendTo(content);
        }
        if (!pager || pager.gridMode === 0) {
          row.addClass('grb-item');
          row.append(content);
          tcontainer.append(row);
        } else {
          content.addClass('grb-item');
          tcontainer.append(content);
        }
      }
    } else {
      row = $('<tr></tr>');
      content = $('<td></td>');
      $('<span>No Items found</span>').appendTo(content);
      row.append(content);
      tcontainer.append(row);
    }

    const tfoot = this.buildFooter(pager);
    table.append(tfoot);

    this.element.append(table);
    this.element.addClass('with-data');
  },

  buildFooter: function (pager) {
    const tfoot = $('<tfoot><tr><td></td></tr></tfoot>');
    const grbPager = $('<nav class="grb-pager"></nav>');
    const list = $('<ul class="pagination"></ul>');

    if (pager && pager.onItemsPerPage && pager.ippOptions) {
      const ippSelect = $('<select name="ippSelect" id="ippSelect" style="margin-right: 10px" class="page-link page-select"></select>');

      const ippOptions = pager.ippOptions;
      const pageParams = {
        itemsPerPage: pager.itemsPerPage,
        gridMode: pager.gridMode,
        totalMatches: pager.totalMatches,
        parentId: pager.parentId
      };
      for (let ipp in ippOptions) {
        const ippOption = $('<option' + (pager.itemsPerPage === ippOptions[ipp] ? ' selected="selected" ' : ' ') + 'value="' + ippOptions[ipp] + '">' + ippOptions[ipp] + '</option>' );
        ippOption.appendTo(ippSelect);
      }
      $('<option' + (pager.itemsPerPage === 0 ? ' selected="selected" ' : ' ') + 'value="0">All</option>').appendTo(ippSelect);
      list.append(ippSelect);
      ippSelect.change(function () { pager.onItemsPerPage(pageParams, Number.parseInt(ippSelect.val())); });
    }
    if (pager && pager.onModeSelect) {
      const gridModes = [
        { id: 0, label: 'Table' },
        { id: 1, label: 'Grid' },
        { id: 2, label: 'Large Grid' },
      ];
      const pageParams = {
        pageNumber: pager.currentPage,
        itemsPerPage: pager.itemsPerPage,
        gridMode: pager.gridMode,
        totalMatches: pager.totalMatches,
        parentId: pager.parentId
      };
      const gmSelect = $('<select name="gridSelect" id="gridSelect" style="margin-right: 10px" class="page-link page-select"></select>');
      for (let gm in gridModes) {
        const gridModeOption = $('<option' + (pager.gridMode === gridModes[gm].id ? ' selected="selected" ' : ' ') + 'value="' + gridModes[gm].id + '">' + gridModes[gm].label + '</option>' );
        gridModeOption.appendTo(gmSelect);
      }
      list.append(gmSelect);
      gmSelect.change(function () { pager.onModeSelect(pageParams, Number.parseInt(gmSelect.val())); });
    }

    if (pager && pager.pageCount && pager.itemsPerPage > 0) {
      const previous = $('<li class="page-item">' +
          '<a class="page-link" aria-label="Previous">' +
          '<span aria-hidden="true">&laquo;</span>' +
          '<span class="sr-only">Previous</span></a>' +
          '</li>');
      const next = $('<li class="page-item">' +
        '<a class="page-link" aria-label="Next">' +
        '<span aria-hidden="true">&raquo;</span>' +
        '<span class="sr-only">Next</span></a>' +
        '</li>');
      const maxPages = Math.ceil(pager.totalMatches / pager.itemsPerPage);
      if (maxPages > 1) {
        if (pager.onNext) {
          const pageParams = {
            itemsPerPage: pager.itemsPerPage,
            gridMode: pager.gridMode,
            totalMatches: pager.totalMatches,
            parentId: pager.parentId
          };
          next.find('a').click(pageParams, pager.onNext);
        }

        list.append(previous);
        if (pager.onPrevious && pager.currentPage > 1) {
          const pageParams = {
            itemsPerPage: pager.itemsPerPage,
            gridMode: pager.gridMode,
            totalMatches: pager.totalMatches,
            parentId: pager.parentId
          };
          previous.find('a').click(pageParams, pager.onPrevious);
        } else {
          previous.addClass('disabled');
        }

        for (let page = 1; page <= maxPages; page++) {
          const pageItem = $('<li class="page-item"></li>');
          const pageLink = $('<a class="page-link">' + page + '</a>');
          pageLink.appendTo(pageItem);

          if (pager.onClick) {
            const pageParams = {
              pageNumber: page,
              gridMode: pager.gridMode,
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
        if (pager.currentPage >= maxPages) {
          next.addClass('disabled');
        }
      }
    }
    grbPager.append(list);
    tfoot.find('td').append(grbPager);

    return tfoot;
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-dataitems');
    this.element.removeClass('with-data');
  }

});
