/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.items.js - this file is part of Gerbera.

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
$.widget('grb.dataitems', {

  _create: function () {
    const data = this.options.data;
    const onDelete = this.options.onDelete;
    const onEdit = this.options.onEdit;
    const onDownload = this.options.onDownload;
    const onAdd = this.options.onAdd;
    let itemType = this.options.itemType;
    const pager = this.options.pager;

    this.element.html('');
    this.element.addClass('grb-dataitems'); // signals module to destroy first
    this.element.addClass('grb-dataitems-' + itemType);
    const table = $('<table></table>').addClass('table').addClass('grb-table-' + itemType);
    const tbody = $('<tbody></tbody>');
    let tcontainer = tbody;
    let row, content, text;

    if (data.length === 0) {
      /* Show a friendly message if there are no rows */
      this.element.append("<h5 class=\"mt-4 text-center text-muted\">Empty</h5>" +
        "<h6 class=\"mt-0 text-center text-muted\">Please pick another container from the tree</h6>");
      return;
    }

    if (itemType === 'search')
      itemType = 'db';

    if (data.length > 0) {
      if (itemType === 'db' && pager && pager.gridMode > 0) {
        row = $('<tr></tr>').addClass('grb-item-row-' + pager.gridMode);
        table.addClass('grb-item-table-' + pager.gridMode);
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
          case 3:
            content = $('<div class="grb-item-grid grb-item-grid-item justify-content-center align-items-center"></div>');
            break;
          default:
            content = $('<td></td>');
            row = $('<tr class="datagrid-row"></tr>').addClass('grb-item-row-' + gm);
        }

        var itemText = item.text;
        if (item.track) {
          itemText = item.track + ' ' + itemText;
        }
        if (item.part) {
          itemText = item.part + ' ' + itemText;
        }
        if (item.index) {
          itemText = item.index + ' ' + itemText;
        }

        if (item.url) {
          text = $('<a></a>');
          text.attr('title', 'Open ' + item.text);
          text.attr('type', item.mtype);
          text.attr('href', item.url);
          if (!pager || pager.gridMode === 0) {
            const details = $('<td></td>');
            const detailsDiv = $('<div></div>');
            detailsDiv.appendTo(details);
            detailsDiv.addClass('pull-right');
            const props = [];
            if (item.size) props.push(item.size);
            if (item.duration) props.push(item.duration);
            if (item.resolution) props.push(item.resolution);
            if (props.length > 0) detailsDiv.text('(' + props.join(', ') + ')');
            else detailsDiv.text('');
            text.text(itemText);
            details.appendTo(row);
            text.appendTo(content);
          } else {
            const gridDiv = $('<div style="display: grid;"></div>');
            text.appendTo(gridDiv);
            if (pager.gridMode === 2) {
              const text2 = $('<a></a>');
              text2.attr('title', 'Open ' + item.text);
              text2.attr('type', item.mtype);
              text2.attr('href', item.url);
              text2.text(itemText);
              text2.appendTo(gridDiv);
            }
            gridDiv.appendTo(content);
          }
        } else {
          text = $('<span></span>');
          text.text(itemText).appendTo(content);
        }
        if (item.image) {
          if (pager && pager.gridMode === 3) {
            if (item.upnp_class.startsWith("object.item.imageItem"))
              text.prepend($('<img class="pull-left grb-image" src="' + item.url + '"/>'));
            else
              text.prepend($('<img class="pull-left grb-image" src="' + item.image + '"/>'));
          } else {
            text.prepend($('<img class="pull-left rounded grb-thumbnail" src="' + item.image + '"/>'));
          }
        } else {
          let icon = "fa-file-o";
          if (item.upnp_class) {
            if (item.upnp_class.startsWith("object.item.audioItem")) {
              icon = "fa-music";
            } else if (item.upnp_class.startsWith("object.item.videoItem")) {
              icon = "fa-film";
            } else if (item.upnp_class.startsWith("object.item.imageItem")) {
              icon = "fa-camera";
            } else if (item.upnp_class.startsWith("object.container")) {
              icon = "fa-folder-o";
            }
          }
          text.prepend($('<div class="d-flex pull-left rounded grb-thumbnail justify-content-center align-items-center"><i class="grb-item-icon text-muted fa ' + icon + '"></i></div>'));
        }
        text.addClass('grb-item-url');

        let buttons;
        if (!pager || pager.gridMode === 0) {
          const buttonsCell = $('<td></td>').addClass('grb-item-buttons');
          buttons = $('<div></div>');
          buttons.appendTo(buttonsCell);
          buttonsCell.appendTo(row);
        } else {
          buttons = $('<div></div>');
          buttons.appendTo(content);
        }
        if (itemType === 'db') {
          buttons.addClass('pull-right justify-content-center align-items-center');
          if (!item.upnp_class || !item.upnp_class.startsWith("object.container")) {
            const downloadIcon = $('<span></span>');
            downloadIcon.prop('title', 'Download item');
            downloadIcon.addClass('grb-item-download fa fa-download');
            downloadIcon.appendTo(buttons);
            if (onDownload) {
              downloadIcon.click(item, onDownload);
            }
          }

          if (!pager || pager.gridMode === 0) {
            const editIcon = $('<span></span>');
            editIcon.prop('title', 'Edit item');
            editIcon.addClass('grb-item-edit fa fa-pencil');
            editIcon.appendTo(buttons);
            if (onEdit) {
              editIcon.click(item, onEdit);
            }
            if (!item.upnp_class || !item.upnp_class.startsWith("object.container")) {
              const deleteIcon = $('<span></span>');
              deleteIcon.prop('title', 'Delete item');
              deleteIcon.addClass('grb-item-delete fa fa-trash-o');
              deleteIcon.appendTo(buttons);
              if (onDelete) {
                deleteIcon.click(item, function (event) {
                  row.remove();
                  onDelete(event);
                });
              }
            }
          }
        } else if (itemType === 'fs') {
          buttons.addClass('grb-item-buttons pull-right');

          const addIcon = $('<span></span>');
          addIcon.prop('title', 'Add item');
          addIcon.addClass('grb-item-add fa fa-plus');
          addIcon.appendTo(buttons);
          if (onAdd) {
            addIcon.click(item, onAdd);
          }
        }
        if (!pager || pager.gridMode === 0) {
          row.addClass('grb-item').addClass('grb-item-row-0');
          row.prepend(content);
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

    if (itemType === 'db') {
      const tfoot = this.buildFooter(pager);
      table.append(tfoot);
    }

    this.element.append(table);
    const activePagerItem = this.element.find('#activePagerItem');
    if (activePagerItem && activePagerItem[0] && activePagerItem[0].nextSibling) {
      activePagerItem[0].nextSibling.scrollIntoView({ behavior: 'smooth' });
    }
    this.element.addClass('with-data');
  },

  buildFooter: function (pager) {
    const tfoot = $('<tfoot><tr><td></td></tr></tfoot>');
    const grbPager = $('<nav class="grb-pager" style="display: flex"></nav>');

    const itemsPerPage = pager && pager.gridMode === 3 ? 1 : (pager && pager.itemsPerPage ? pager.itemsPerPage : 0);
    if (pager && pager.onItemsPerPage && pager.ippOptions) {
      const list = $('<ul class="pagination"></ul>');
      const ippSelect = $('<select ' + (pager.gridMode === 3 ? 'hidden ' : '') + 'name="ippSelect" id="ippSelect" style="margin-right: 10px" class="page-link page-select"></select>');

      const ippOptions = pager.ippOptions.option;
      const pageParams = {
        itemsPerPage: pager.itemsPerPage,
        gridMode: pager.gridMode,
        totalMatches: pager.totalMatches,
        parentId: pager.parentId
      };
      for (let ipp in ippOptions) {
        const ippOption = $('<option' + (pager.itemsPerPage === ippOptions[ipp] ? ' selected="selected" ' : ' ') + 'value="' + ippOptions[ipp] + '">'
          + ippOptions[ipp] + ((pager.ippOptions.default === ippOptions[ipp]) ? ' *' : '') + '</option>');
        ippOption.appendTo(ippSelect);
      }
      $('<option' + (pager.itemsPerPage === 0 ? ' selected="selected" ' : ' ') + 'value="0">All</option>').appendTo(ippSelect);
      list.append(ippSelect);
      ippSelect.change(function () { pager.onItemsPerPage(pageParams, Number.parseInt(ippSelect.val())); });
      grbPager.append(list);
    }
    if (pager && pager.onModeSelect) {
      const list = $('<ul class="pagination"></ul>');
      const gridModes = [
        { id: 0, label: 'Table' },
        { id: 1, label: 'Grid' },
        { id: 2, label: 'Large Grid' },
        { id: 3, label: 'Item' },
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
        const gridModeOption = $('<option' + (pager.gridMode === gridModes[gm].id ? ' selected="selected" ' : ' ') + 'value="' + gridModes[gm].id + '">' + gridModes[gm].label + '</option>');
        gridModeOption.appendTo(gmSelect);
      }
      list.append(gmSelect);
      gmSelect.change(function () { pager.onModeSelect(pageParams, Number.parseInt(gmSelect.val())); });
      grbPager.append(list);
    }

    if (pager && pager.pageCount && itemsPerPage > 0) {
      let list = $('<ul class="pagination"></ul>');
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
      const previous10 = $('<li class="page-item">' +
        '<a class="page-link" aria-label="Previous">' +
        '<span aria-hidden="true">&laquo;&laquo;</span>' +
        '<span class="sr-only">Previous</span></a>' +
        '</li>');
      const next10 = $('<li class="page-item">' +
        '<a class="page-link" aria-label="Next">' +
        '<span aria-hidden="true">&raquo;&raquo; </span>' +
        '<span class="sr-only">Next</span></a>' +
        '</li>');
      const maxPages = Math.ceil(pager.totalMatches / itemsPerPage);
      const maxPages10 = Math.ceil(pager.totalMatches / itemsPerPage / 10);
      if (maxPages > 1) {
        if (maxPages10 > 1) {
          list.append(previous10);
          if (pager.onPrevious && pager.currentPage > 10) {
            const pageParams = {
              itemsPerPage: pager.itemsPerPage,
              gridMode: pager.gridMode,
              totalMatches: pager.totalMatches,
              parentId: pager.parentId,
              increment: 10
            };
            previous10.find('a').click(pageParams, pager.onPrevious);
          } else {
            previous10.addClass('disabled');
          }
        }
        list.append(previous);
        grbPager.append(list);
        if (pager.onPrevious && pager.currentPage > 1) {
          const pageParams = {
            itemsPerPage: pager.itemsPerPage,
            gridMode: pager.gridMode,
            totalMatches: pager.totalMatches,
            parentId: pager.parentId,
            increment: 1
          };
          previous.find('a').click(pageParams, pager.onPrevious);
        } else {
          previous.addClass('disabled');
        }

        list = $('<ul class="pagination ' + (maxPages10 > 1 ? 'itemspager10' : 'itemspager') + '"></ul>');
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
            pageItem.attr('id', 'activePagerItem');
          }

          list.append(pageItem);
        }
        grbPager.append(list);

        list = $('<ul class="pagination"></ul>');
        if (pager.onNext) {
          const pageParams = {
            itemsPerPage: pager.itemsPerPage,
            gridMode: pager.gridMode,
            totalMatches: pager.totalMatches,
            parentId: pager.parentId,
            increment: 1
          };
          next.find('a').click(pageParams, pager.onNext);
        }
        list.append(next);
        if (maxPages10 > 1) {
          list.append(next10);
          if (pager.onNext && pager.currentPage < (maxPages10 - 1) * 10) {
            const pageParams = {
              itemsPerPage: pager.itemsPerPage,
              gridMode: pager.gridMode,
              totalMatches: pager.totalMatches,
              parentId: pager.parentId,
              increment: 10
            };
            next10.find('a').click(pageParams, pager.onNext);
          } else {
            next10.addClass('disabled');
          }
        }
        if (pager.currentPage >= maxPages) {
          next.addClass('disabled');
        }
      }
      grbPager.append(list);
    }
    tfoot.find('td').append(grbPager);

    return tfoot;
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-dataitems');
    this.element.removeClass('with-data');
  }

});
