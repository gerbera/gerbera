/* global GERBERA spyOn jasmine it expect describe beforeEach loadFixtures loadJSONFixtures getJSONFixture */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('Gerbera Menu', () => {
  'use strict';
  describe('initialize()', () => {
    let ajaxSpy, mockConfig;

    beforeEach(() => {
      loadJSONFixtures('config.json');
      loadFixtures('index.html');
      mockConfig = getJSONFixture('config.json');
      spyOn(GERBERA.Updates, 'getUpdates');
      ajaxSpy = spyOn($, 'ajax');
      GERBERA.App.serverConfig = mockConfig.config;
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('binds all menu items with the click event', async () => {
      spyOn(GERBERA.Menu, 'click');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);

      await GERBERA.Menu.initialize();

      $('#nav-db').click();
      expect(GERBERA.Menu.click).toHaveBeenCalledWith(jasmine.any(Object));
    });

    it('on click of Database calls to load the containers', async () => {
      spyOn(GERBERA.Tree, 'selectType');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);

      await GERBERA.Menu.initialize();
      $('#nav-db').click();

      expect(GERBERA.Tree.selectType).toHaveBeenCalledWith('db', 0);
    });

    it('on click of menu, clears items', async () => {
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.Items, 'destroy');
      spyOn(GERBERA.Tree, 'selectType');

      loadJSONFixtures('parent_id-0-select_it-0.json');
      const response = getJSONFixture('parent_id-0-select_it-0.json');
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(response).promise();
      });

      await GERBERA.Menu.initialize();
      $('#nav-db').click();

      expect(GERBERA.Items.destroy).toHaveBeenCalled();
    });

    it('on click of home menu, clears tree and items', async () => {
      spyOn(GERBERA.Items, 'destroy');
      spyOn(GERBERA.Tree, 'destroy');
      spyOn(GERBERA.Trail, 'destroy');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);

      await GERBERA.Menu.initialize();
      $('#nav-home').click();

      expect(GERBERA.Items.destroy).toHaveBeenCalled();
      expect(GERBERA.Tree.destroy).toHaveBeenCalled();
      expect(GERBERA.Trail.destroy).toHaveBeenCalled();
    });
  });

  describe('disable()', () => {

    beforeEach(() => {
      loadFixtures('index.html');
      spyOn(GERBERA.Updates, 'getUpdates');
    });

    it('disables all menu items except the report issue link', () => {
      GERBERA.Menu.disable();

      const menuItems = ['nav-home', 'nav-db', 'nav-fs'];

      $.each(menuItems, (index, value) => {
        const link = $('#'+ value);
        expect(link.hasClass('disabled')).toBeTruthy();
      });
      expect( $('#report-issue').hasClass('disabled')).toBeFalsy();
    });
  });

  describe('hideLogin()', () => {

    beforeEach(() => {
      loadFixtures('index.html');
      spyOn(GERBERA.Updates, 'getUpdates');
    });

    it('hides login fields when called', () => {
      GERBERA.Menu.hideLogin();

      expect($('.login-field').is(':visible')).toBeFalsy();
      expect($('#login-submit').is(':visible')).toBeFalsy();
      expect($('#logout').is(':visible')).toBeFalsy();
    });
  });

  describe('click()', () => {
    let fsMenu;
    let mockConfig;

    beforeEach(async () => {
      loadFixtures('index.html');
      loadJSONFixtures('config.json');
      spyOn(GERBERA.Tree, 'selectType');
      spyOn(GERBERA.App, 'setType');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.Items, 'destroy');
      spyOn(GERBERA.Tree, 'destroy');
      spyOn(GERBERA.Trail, 'destroy');
      mockConfig = getJSONFixture('config.json');
      GERBERA.App.serverConfig = mockConfig.config;
      await GERBERA.Menu.initialize();
      fsMenu = $('#nav-fs');
    });

    it('sets the active menu item when clicked', () => {
      fsMenu.click();

      expect(fsMenu.parent().hasClass('active')).toBeTruthy();
      expect(GERBERA.Tree.selectType).toHaveBeenCalled();
    });

    it('sets the correct active menu item parent when icon is clicked', () => {
      const fsIcon = fsMenu.children('i.fa-folder-open');

      fsIcon.click();

      expect(fsMenu.parent().hasClass('active')).toBeTruthy();
      expect(GERBERA.Tree.selectType).toHaveBeenCalled();
    });
  });
});
