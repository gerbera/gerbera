import datagridData from './fixtures/datagrid-data';

describe('The jQuery Datagrid', () => {
  'use strict';
  let dataGrid;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    dataGrid = $('#datagrid');
  });

  afterEach(() => {
    fixture.cleanup();
  });

  it('creates a table based on the dataset provided', () => {
    dataGrid.dataitems({
      data: datagridData
    });

    expect(dataGrid.find('tr').length).toBe(11);
    expect(dataGrid.find('tr.grb-item').get(0).innerText).toContain(datagridData[0].text); // skip the header row
    expect(dataGrid.find('a').get(0).href).toBe(datagridData[0].url);
  });

  it('binds the delete icon click to the method provided and removes the row on click', () => {
    const methodSpy = jasmine.createSpy('delete');
    dataGrid.dataitems({
      data: datagridData,
      onDelete: methodSpy,
      itemType: 'db'
    });
    const beforeCount = dataGrid.find('.grb-item').length;

    dataGrid.find('.grb-item span.grb-item-delete').first().click();

    const afterCount = dataGrid.find('.grb-item').length;
    expect(afterCount).toEqual(beforeCount - 1);
    expect(methodSpy).toHaveBeenCalled();
  });

  it('does not bind delete on file system types', () => {
    const methodSpy = jasmine.createSpy('delete');
    dataGrid.dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'fs'
    });

    dataGrid.find('.grb-item span.grb-item-delete').first().click();

    expect(methodSpy).not.toHaveBeenCalled();
  });

  it('binds the edit icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('edit');
    dataGrid.dataitems({
      data: datagridData,
      onEdit: methodSpy,
      itemType: 'db'
    });

    dataGrid.find('.grb-item span.grb-item-edit').first().click();

    expect(methodSpy).toHaveBeenCalled();
  });

  it('does not bind edit on file system types', () => {
    const methodSpy = jasmine.createSpy('edit');
    dataGrid.dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'fs'
    });

    dataGrid.find('.grb-item span.grb-item-edit').first().click();

    expect(methodSpy).not.toHaveBeenCalled();
  });

  it('binds the download icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('download');
    dataGrid.dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'db'
    });

    dataGrid.find('.grb-item span.grb-item-download').first().click();

    expect(methodSpy).toHaveBeenCalled();
  });

  it('does not bind download on file system types', () => {
    const methodSpy = jasmine.createSpy('download');
    dataGrid.dataitems({
      data: datagridData,
      onDownload: methodSpy,
      itemType: 'fs'
    });

    dataGrid.find('.grb-item span.grb-item-download').first().click();

    expect(methodSpy).not.toHaveBeenCalled();
  });

  it('binds the add icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('add');
    dataGrid.dataitems({
      data: datagridData,
      onAdd: methodSpy,
      itemType: 'fs'
    });

    dataGrid.find('.grb-item span.grb-item-add').first().click();

    expect(methodSpy).toHaveBeenCalled();
  });

  it('does not bind add icon to db types', () => {
    const methodSpy = jasmine.createSpy('add');
    dataGrid.dataitems({
      data: datagridData,
      onAdd: methodSpy,
      itemType: 'db'
    });

    dataGrid.find('.grb-item span.grb-item-add').first().click();

    expect(methodSpy).not.toHaveBeenCalled();
  });

  it('creates a table with a footer container a pager', () => {
    dataGrid.dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 1,
        totalMatches: 1000,
        itemsPerPage: 10}
    });

    expect(dataGrid.find('tfoot').length).toBe(1);
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(102);
  });

  it('creates a pager with only page count pages', () => {
    dataGrid.dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 1,
        totalMatches: 1000,
        itemsPerPage: 10}
    });

    expect(dataGrid.find('tfoot').length).toBe(1);
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(102);
  });

  it('creates a pager with only necessary amount of pages', () => {
    dataGrid.dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 1,
        totalMatches: 20,
        itemsPerPage: 10}
    });

    expect(dataGrid.find('tfoot').length).toBe(1);
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(4);
  });

  it('creates a pager with existing number of pages', () => {
    dataGrid.dataitems({
      data: datagridData,
      pager: {pageCount: 10,
        currentPage: 2,
        totalMatches: 20,
        itemsPerPage: 10}
    });

    expect(dataGrid.find('tfoot').length).toBe(1);
    expect($('#datagrid nav.grb-pager > ul > li').length).toBe(4);
  });

  it('does not create a pager when no page info exists', () => {
    dataGrid.dataitems({
      data: datagridData,
      pager: {pageCount: 0}
    });

    expect($('#datagrid nav.grb-pager').length).toBe(0);
  });

  it('does not create a pager when no page info exists', () => {
    dataGrid.dataitems({
      data: datagridData
    });

    expect($('#datagrid nav.grb-pager').length).toBe(0);
  });

  it('creates a pager that goes to specific page when clicked', () => {
    const clickSpy = jasmine.createSpy('click');
    dataGrid.dataitems({
      data: datagridData,
      pager: {
        currentPage: 1,
        pageCount: 10,
        onClick: clickSpy,
        totalMatches: 1000,
        itemsPerPage: 10
      }
    });

    $($('#datagrid nav.grb-pager > ul > li').get(1)).find('a').click();

    expect(clickSpy).toHaveBeenCalled();
  });

  it('creates a pager that goes to previous page when clicked', () => {
    const clickSpy = jasmine.createSpy('click');
    const nextSpy = jasmine.createSpy('next');
    const previousSpy = jasmine.createSpy('previous');
    dataGrid.dataitems({
      data: datagridData,
      pager: {
        currentPage: 1,
        pageCount: 10,
        onClick: clickSpy,
        onNext: nextSpy,
        onPrevious: previousSpy,
        totalMatches: 1000,
        itemsPerPage: 10
      }
    });

    // the << previous directional
    $($('#datagrid nav.grb-pager > ul > li').get(0)).find('a').click();

    expect(previousSpy).toHaveBeenCalled();
  });

  it('creates a pager that goes to next page when clicked', () => {
    const clickSpy = jasmine.createSpy('click');
    const nextSpy = jasmine.createSpy('next');
    const previousSpy = jasmine.createSpy('previous');
    dataGrid.dataitems({
      data: datagridData,
      pager: {
        currentPage: 1,
        pageCount: 10,
        onClick: clickSpy,
        onNext: nextSpy,
        onPrevious: previousSpy,
        totalMatches: 20,
        itemsPerPage: 10
      }
    });

    // the >> next directional
    $($('#datagrid nav.grb-pager > ul > li').get(3)).find('a').click();

    expect(nextSpy).toHaveBeenCalled();
  });
});
