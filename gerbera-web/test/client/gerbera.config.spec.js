/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.config.spec.js - this file is part of Gerbera.

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
import { GerberaApp } from "../../../web/js/gerbera-app.module";
import { Config } from '../../../web/js/gerbera-config.module';
import mockConfig from './fixtures/config';
import configMetaJson from './fixtures/config_meta';
import configValueJson from './fixtures/config_values';

describe('Gerbera Config', () => {
  let lsSpy;
  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    lsSpy = spyOn(window.localStorage, 'getItem').and.callFake((name) => {
      return;
    });
  });
  afterEach((done) => {
    fixture.cleanup();
    done();
  });

  describe('initialize()', () => {
    beforeEach(() => {
      GerberaApp.serverConfig = mockConfig.config;
    });

    it('clears the datagrid', async () => {
      await Config.initialize();
      expect($('#configgrid').text()).toBe('');
    });
  });

  describe('loadItems()', () => {
    beforeEach(() => {
      GerberaApp.serverConfig = {};
      spyOn(GerberaApp, 'getType').and.returnValue('config');
      spyOn(GerberaApp, 'configMode').and.returnValue('minimal');
    });

    it('does not load items if response is failure', async () => {
      await Config.initialize();
      configMetaJson.success = false;
      Config.loadConfig(configMetaJson, 'config');
      Config.loadConfig(configValueJson, 'values');
      Config.loadConfig(configValueJson, 'meta');
      expect($('#configgrid').find('li').length).toEqual(0);
      configMetaJson.success = true;
    });

    it('loads the response as tree in the grid', async () => {
      await Config.initialize();
      configMetaJson.success = true;
      Config.loadConfig(configMetaJson, 'config');
      Config.loadConfig(configValueJson, 'values');
      Config.loadConfig(configValueJson, 'meta');
      expect($('#configgrid').find('li').length).toEqual(167);
      configMetaJson.success = true;
    });

    it('has the correct serial number in the server config', async () => {
      await Config.initialize();
      configMetaJson.success = true;
      Config.loadConfig(configMetaJson, 'config');
      Config.loadConfig(configValueJson, 'values');
      Config.loadConfig(configValueJson, 'meta');
      let node = $('#configgrid').find('li')[0];
      node.click();
      expect($('#grb_value__server_serialNumber').val()).toEqual('42');
      configMetaJson.success = true;
    });
  });
});
