/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.clients.js - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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
$.widget('grb.clients', {

  _create: function () {
    this.element.html('');
    this.element.addClass('grb-clients');
    const table = $('<table></table>').addClass('table');
    const clientHeadings = {
      ip: 'IP Address',
      time: 'First Seen',
      last: 'Last Seen',
      host: 'Host Name',
      group: 'Group',
      name: 'Profile',
      userAgent: 'User Agent',
      matchType: 'Match Type',
      match: 'Match Pattern',
      clientType: 'Client Type',
      flags: 'Client Flags',
      empty: '',
    };
    const clientProps = [['ip', 'host', 'group'], 'userAgent', ['name', 'matchType', 'match'], 'clientType', ['time', 'last'], 'flags'];
    this.buildTable(table, this.options.data, clientHeadings, clientProps, 'Clients', clientProps.length, this.options.onDelete);

    const groupHeadings = {
      name: 'Group',
      count: 'Count',
      playCount: 'Play Count',
      bookmarks: 'Bookmarks',
      last: 'Last Action',
      empty: '',
    };
    const groupProps = ['name', 'count', 'playCount', 'bookmarks', 'last', 'empty', 'empty'];
    this.buildTable(table, this.options.groups, groupHeadings, groupProps, 'Groups', clientProps.length);
    this.element.append(table);
    this.element.addClass('with-data');
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-clients');
    this.element.removeClass('with-data');
  },

  buildTable: function (table, data, headings, props, caption, size, onDelete) {
    const tbody = $('<tbody></tbody>');
    const thead = $('<thead></thead>');
    let row, content, text;

    if (data && data.length > 0) {
      row = $('<tr></tr>');
      props.forEach(function (prop) {
        content = $('<th></th>');
        const pL = (Array.isArray(prop)) ? prop : [prop];
        pL.forEach(function (p, idx) {
          if (idx > 0)
            $('<br><hr>').appendTo(content);
          text = $('<span></span>');
          text.text(headings[p]).appendTo(content);
          content.addClass('grb-client-' + p);
        });
        row.append(content);
      });
      if (onDelete) {
        content = $('<th></th>');
        const deleteIcon = $('<span>Action</span>');
        deleteIcon.addClass('grb-client-delete');
        deleteIcon.appendTo(content);
        row.append(content);
      }

      row.addClass('grb-client');
      thead.append(row);

      for (let i = 0; i < data.length; i++) {
        const item = data[i];
        row = $('<tr></tr>');

        props.forEach(function (prop) {
          content = $('<td></td>');
          const pL = (Array.isArray(prop)) ? prop : [prop];
          pL.forEach(function (p, idx) {
            if (idx > 0)
              $('<br><hr>').appendTo(content);
            text = $('<span></span>');
            text.text(item[p]).appendTo(content);
            content.addClass('grb-client-' + p);
          });
          row.append(content);
        });
        if (onDelete) {
          content = $('<td></td>');
          const deleteIcon = $('<span></span>');
          deleteIcon.prop('title', 'Delete client');
          deleteIcon.addClass('grb-client-delete fa fa-trash-o');
          deleteIcon.appendTo(content);
          row.append(content);
          deleteIcon.click(item, function (event) {
            row.remove();
            onDelete(event.data);
          });
        }
        row.addClass('grb-client');
        tbody.append(row);
      }
      thead.appendTo(table);
    } else {
      row = $('<tr></tr>');
      content = $('<td colspan="' + size + '"></td>');
      $('<span>No ' + caption + ' found</span>').appendTo(content);
      row.append(content);
      tbody.append(row);
    }

    tbody.appendTo(table);
  }
});
