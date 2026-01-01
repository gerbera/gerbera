/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.clients.spec.js - this file is part of Gerbera.

    Copyright (C) 2016-2026 Gerbera Contributors

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
/* global fixture */
import datagridData from './fixtures/clientgrid-data';

describe('The jQuery Clientgrid', () => {
  'use strict';
  let dataGrid;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    dataGrid = $('#clientgrid');
  });

  afterEach(() => {
    fixture.cleanup();
  });

  it('creates a table based on the dataset provided', () => {
    dataGrid.clients({
      data: datagridData
    });

    expect(dataGrid.find('tr').length).toBe(4);
    expect(dataGrid.find('tr.grb-client').get(1).innerText).toContain(datagridData[0].ip);
  });
});
