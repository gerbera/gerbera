import {Items} from "../../../web/js/gerbera-items.module";
import {GerberaApp} from '../../../web/js/gerbera-app.module';
import {Updates} from "../../../web/js/gerbera-updates.module";
import {Menu} from '../../../web/js/gerbera-menu.module';
import {Tree} from "../../../web/js/gerbera-tree.module";
import {Trail} from "../../../web/js/gerbera-trail.module";
import mockConfig from './fixtures/config';
import treeMock from './fixtures/parent_id-0-select_it-0';

describe('Gerbera Menu', () => {
  describe('initialize()', () => {
    let ajaxSpy;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      spyOn(Updates, 'getUpdates');
      ajaxSpy = spyOn($, 'ajax');
      GerberaApp.serverConfig = mockConfig.config;
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.and.callThrough();
    });

    it('binds all menu items with the click event', async () => {
      spyOn(Menu, 'click');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);

      await Menu.initialize();

      $('#nav-db').click();
      expect(Menu.click).toHaveBeenCalledWith(jasmine.any(Object));
    });

    it('on click of Database calls to load the containers', async () => {
      spyOn(Tree, 'selectType');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);

      await Menu.initialize();
      $('#nav-db').click();

      expect(Tree.selectType).toHaveBeenCalledWith('db', 0);
    });

    it('on click of menu, clears items', async () => {
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);
      spyOn(Items, 'destroy');
      spyOn(Tree, 'selectType');
      ajaxSpy.and.callFake(() => {
        return Promise.resolve(treeMock);
      });

      await Menu.initialize();
      $('#nav-db').click();

      expect(Items.destroy).toHaveBeenCalled();
    });

    it('on click of home menu, clears tree and items', async () => {
      spyOn(Items, 'destroy');
      spyOn(Tree, 'destroy');
      spyOn(Trail, 'destroy');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);

      await Menu.initialize();
      $('#nav-home').click();

      expect(Items.destroy).toHaveBeenCalled();
      expect(Tree.destroy).toHaveBeenCalled();
      expect(Trail.destroy).toHaveBeenCalled();
    });
  });
  describe('disable()', () => {
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      spyOn(Updates, 'getUpdates');
    });
    afterEach(() => {
      fixture.cleanup();
    });
    it('disables all menu items except the report issue link', () => {
      Menu.disable();

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
      spyOn(Updates, 'getUpdates');
    });
    afterEach(() => {
      fixture.cleanup();
    });

    it('hides login fields when called', () => {
      Menu.hideLogin();

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
      spyOn(Tree, 'selectType');
      spyOn(GerberaApp, 'setType');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);
      spyOn(Items, 'destroy');
      spyOn(Tree, 'destroy');
      spyOn(Trail, 'destroy');

      GerberaApp.serverConfig = mockConfig.config;
      await Menu.initialize();
      fsMenu = $('#nav-fs');
    });
    afterEach(() => {
      fixture.cleanup();
    });

    it('sets the active menu item when clicked', () => {
      fsMenu.click();

      expect(fsMenu.parent().hasClass('active')).toBeTruthy();
      expect(Tree.selectType).toHaveBeenCalled();
    });

    it('sets the correct active menu item parent when icon is clicked', () => {
      const fsIcon = fsMenu.children('i.fa-folder-open');

      fsIcon.click();

      expect(fsMenu.parent().hasClass('active')).toBeTruthy();
      expect(Tree.selectType).toHaveBeenCalled();
    });
  });
});