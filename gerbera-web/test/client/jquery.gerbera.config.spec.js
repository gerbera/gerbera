/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.config.spec.js - this file is part of Gerbera.

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
import configMetaJson from './fixtures/config_meta';
import configValueJson from './fixtures/config_values';

describe('The jQuery Configgrid', () => {
  'use strict';
  let dataGrid;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    dataGrid = $('#configgrid');
  });

  afterEach(() => {
    fixture.cleanup();
  });

  it('creates a tree based on config and values', () => {
    dataGrid.config({
      setup: configMetaJson.config,
      values: configValueJson.values,
      meta: configValueJson.meta,
      choice: 'standard',
      chooser: {
        minimal: { caption: 'Minimal', fileName: './fixtures/config_meta.json' }
      },
      addResultItem: null,
      configModeChanged: null,
      itemType: 'config'
    });

    expect(dataGrid.find('ul').length).toBe(67);
    expect(dataGrid.find('li.grb-config').length).toBe(116);
    expect(dataGrid.find('li.grb-config').get(8).innerText).toContain('Model Number');
    expect(dataGrid.find('#grb_line__server_modelNumber').find('input').get(0).value).toContain('2.2.0');
  });
});
