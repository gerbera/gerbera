/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.clients.js - this file is part of Gerbera.

    Copyright (C) 2016-2023 Gerbera Contributors

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
      flags: 'Quirk Flags'
    };
    const clientProps = ['ip', 'host', 'group', 'name', 'userAgent', 'matchType', 'match', 'clientType', 'time', 'last', 'flags' ];
    this.buildTable (table, this.options.data, clientHeadings, clientProps, 'Clients', clientProps.length);

    const groupHeadings = {
      name: 'Group',
      count: 'Count',
      playCount: 'Play Count',
      bookmarks: 'Bookmarks',
      last: 'Last Action',
      empty: '',
    };
    const groupProps = ['empty', 'empty', 'name', 'empty', 'empty', 'count', 'playCount', 'bookmarks', 'empty', 'last', 'empty' ];
    this.buildTable (table, this.options.groups, groupHeadings, groupProps, 'Groups', clientProps.length);
    this.element.append(table);
    this.element.addClass('with-data');
  },

  _destroy: function () {
    this.element.children('table').remove();
    this.element.removeClass('grb-clients');
    this.element.removeClass('with-data');
  },

  buildTable: function (table, data, headings, props, caption, size) {
    const tbody = $('<tbody></tbody>');
    const thead = $('<thead></thead>');
    let row, content, text;

    if (data && data.length > 0) {
      row = $('<tr></tr>');
      props.forEach( function(p) {
          content = $('<th></th>');
          text = $('<span></span>');
          text.text(headings[p]).appendTo(content);
          content.addClass('grb-client-' + p);

          row.append(content);
      });

      row.addClass('grb-client');
      thead.append(row);

      for (let i = 0; i < data.length; i++) {
        const item = data[i];
        row = $('<tr></tr>');

        props.forEach( function(p) {
            content = $('<td></td>');
            text = $('<span></span>');
            text.text(item[p]).appendTo(content);
            content.addClass('grb-client-' + p);
            row.append(content);
        });

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
