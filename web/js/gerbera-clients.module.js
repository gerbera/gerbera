/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-clients.module.js - this file is part of Gerbera.

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
import {GerberaApp} from './gerbera-app.module.js';
import {Auth} from './gerbera-auth.module.js';

const destroy = () => {
  const datagrid = $('#clientgrid');
  if (datagrid.hasClass('grb-clients')) {
    datagrid.clients('destroy');
  } else {
    datagrid.html('');
  }
};

const initialize = () => {
  $('#clientgrid').html('');
  return Promise.resolve();
};

const menuSelected = () => {
  retrieveGerberaItems('clients')
    .then((response) => loadItems(response))
    .catch((err) => GerberaApp.error(err));
};

const retrieveGerberaItems = (type) => {
  return $.ajax({
    url: GerberaApp.clientConfig.api,
    type: 'get',
    data: {
      req_type: type,
      sid: Auth.getSessionId()
     }
  });
};

const loadItems = (response) => {
  if (response.success) {
    let items;

    items = transformItems(response.clients.client);

    const datagrid = $('#clientgrid');

    if (datagrid.hasClass('grb-clients')) {
      datagrid.clients('destroy');
    }

    datagrid.clients({
      data: items,
      itemType: 'clients'
    });
  }
};

const iptoi = (addr) => {
  var parts = addr.split('.').map((str) => { return parseInt(str); });
  
  if (parts.length === 1) { // IPv6
    parts = addr.split(':').map((str) => { return parseInt(str); });
  }
  
  if (parts.length >= 6) {
     return (parts[0] ? parts[0] << 40 : 0) +
         (parts[1] ? parts[1] << 32 : 0) +
         (parts[2] ? parts[2] << 24 : 0) +
         (parts[3] ? parts[3] << 16 : 0) +
         (parts[4] ? parts[4] << 8  : 0) +
          parts[5];
  } else if (parts.length >= 4) {
     return (parts[0] ? parts[0] << 24 : 0) +
         (parts[1] ? parts[1] << 16 : 0) +
         (parts[2] ? parts[2] << 8  : 0) +
          parts[3];
  }
  return 0;
};

const transformItems = (items) => {
  const widgetItems = [];
  const ipFilter = /:[0-9]+$/;

  for (let i = 0; i < items.length; i++) {
    const gItem = items[i];
    gItem.ip = gItem.ip.replace(ipFilter, '');
    if (!widgetItems.some((item) => {
        return item.ip === gItem.ip && item.userAgent === gItem.userAgent;
    })) {
        widgetItems.push(gItem);
    }
  }

  return widgetItems.sort( (a,b) => { return iptoi(a.ip) - iptoi(b.ip) });
};

export const Clients = {
  destroy,
  loadItems,
  initialize,
  transformItems,
  menuSelected,
};
