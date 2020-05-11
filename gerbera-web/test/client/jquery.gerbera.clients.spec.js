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

    expect(dataGrid.find('tr').length).toBe(3);
    expect(dataGrid.find('tr.grb-client').get(0).innerText).toContain(datagridData[0].ip);
  });
});
