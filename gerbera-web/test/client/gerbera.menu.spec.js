import mockConfig from './fixtures/config';
import treeMock from './fixtures/parent_id-0-select_it-0';

describe('Gerbera Menu', () => {
  'use strict';
  describe('initialize()', () => {
    let ajaxSpy;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      spyOn(GERBERA.Updates, 'getUpdates');
      ajaxSpy = spyOn($, 'ajax');
      GERBERA.App.serverConfig = mockConfig.config;
    });

    afterEach(() => {
      fixture.cleanup();
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
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(treeMock).promise();
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
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      spyOn(GERBERA.Updates, 'getUpdates');
    });
    afterEach(() => {
      fixture.cleanup();
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
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      spyOn(GERBERA.Updates, 'getUpdates');
    });
    afterEach(() => {
      fixture.cleanup();
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

    beforeEach(async () => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      spyOn(GERBERA.Tree, 'selectType');
      spyOn(GERBERA.App, 'setType');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.Items, 'destroy');
      spyOn(GERBERA.Tree, 'destroy');
      spyOn(GERBERA.Trail, 'destroy');
      GERBERA.App.serverConfig = mockConfig.config;
      await GERBERA.Menu.initialize();
      fsMenu = $('#nav-fs');
    });
    afterEach(() => {
      fixture.cleanup();
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
