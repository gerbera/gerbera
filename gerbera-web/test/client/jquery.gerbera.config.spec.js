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

  it('creates a tree base on config and values', () => {
    dataGrid.config({
      meta: configMetaJson.config,
      values: configValueJson.values,
      choice: 'standard',
      chooser: {
        minimal: {caption: 'Minimal', fileName: './fixtures/config_meta.json'}
      },
      addResultItem: null,
      configModeChanged: null,
      itemType: 'config'
    });

    expect(dataGrid.find('ul').length).toBe(63);
    expect(dataGrid.find('li.grb-config').get(8).innerText).toContain('Model Number');
    expect(dataGrid.find('#item__server_modelNumber').find('input').get(0).value).toContain('1.7.0');
  });
});
