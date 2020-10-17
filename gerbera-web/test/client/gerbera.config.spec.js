import {GerberaApp} from "../../../web/js/gerbera-app.module";
import {Config} from '../../../web/js/gerbera-config.module';
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
      expect($('#configgrid').find('li').length).toEqual(0);
      configMetaJson.success = true;
    });

    it('loads the response as tree in the grid', async () => {
      await Config.initialize();
      configMetaJson.success = true;
      Config.loadConfig(configMetaJson, 'config');
      Config.loadConfig(configValueJson, 'values');
      expect($('#configgrid').find('li').length).toEqual(150);
      configMetaJson.success = true;
    });

    it('has the correct serial number in the server config', async () => {
      await Config.initialize();
      configMetaJson.success = true;
      Config.loadConfig(configMetaJson, 'config');
      Config.loadConfig(configValueJson, 'values');
      let node = $('#configgrid').find('li')[0];
      node.click();
      expect($('#value__server_serialNumber_8_0').val()).toEqual('42');
      configMetaJson.success = true;
    });
  });
});
