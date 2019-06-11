import {Auth} from '../../../web/js/gerbera-auth.module';
import {Autoscan} from '../../../web/js/gerbera-autoscan.module';
import {GerberaApp} from '../../../web/js/gerbera-app.module';
import {Items} from '../../../web/js/gerbera-items.module';
import {Menu} from '../../../web/js/gerbera-menu.module';
import {Trail} from '../../../web/js/gerbera-trail.module';
import {Tree} from '../../../web/js/gerbera-tree.module';
import {Updates} from '../../../web/js/gerbera-updates.module';

import convertedConfig from './fixtures/converted-config';
import mockConfig from './fixtures/config';
import uiDisabled from './fixtures/ui-disabled';

describe('Gerbera UI App', () => {

  describe('initialize()', () => {
    let ajaxSpy, ajaxSetupSpy, cookieSpy;

    beforeAll(() => {
      ajaxSpy = spyOn(window.$, 'ajax');
      ajaxSetupSpy = spyOn(window.$, 'ajaxSetup');
      spyOn(Updates, 'initialize').and.returnValue(Promise.resolve({}));
      spyOn(Auth, 'checkSID').and.returnValue(Promise.resolve(true));
      spyOn(Auth, 'getSessionId').and.returnValue('A_MOCK_SID');
      spyOn(Menu, 'initialize').and.returnValue(Promise.resolve({}))
    });

    beforeEach(() => {
      cookieSpy = spyOn(Cookies, 'get').and.callFake((name) => {
        if (name === 'TYPE') return 'db';
        if (name === 'SID') return 'A_MOCK_SID';
        return 'A_MOCK_COOKIE';
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
      ajaxSetupSpy.and.callThrough();
    });

    it('creates an instance of Gerbera App', () => {
      expect(GerberaApp).toBeDefined();
    });

    it('provides access to the TYPE cookie', () => {
      const result = GerberaApp.getType();

      expect(result).toEqual('db');
    });

    it('retrieves the configuration from the server using AJAX', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(mockConfig).promise();
      });

      await GerberaApp.initialize();

      expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface');
      expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
        req_type: 'auth',
        sid: 'A_MOCK_SID',
        action: 'get_config'
      });

      expect(GerberaApp.serverConfig).toEqual(convertedConfig.config);
      expect(Auth.checkSID).toHaveBeenCalled();
      expect(Updates.initialize).toHaveBeenCalled();
      expect(Menu.initialize).toHaveBeenCalledWith(GerberaApp.serverConfig);
      expect($(document).attr('title')).toBe('Gerbera Media Server | Gerbera Media Server');
    });

    it('reports an error when the ajax calls fails', async () => {
      spyOn(Updates, 'showMessage');

      ajaxSpy.and.callFake(() => {
        return Promise.reject({responseText: 'Internal Server Error'});
        });

      await GerberaApp.initialize();
      expect(Updates.showMessage).toHaveBeenCalledWith('Internal Server Error', undefined, 'info', 'fa-exclamation-triangle');
    });

    it('reports an error when UI is disabled', async () => {
      spyOn(Updates, 'showMessage');
      ajaxSpy.and.callFake(() => {
        return Promise.resolve(uiDisabled);
      });

      await GerberaApp.initialize();

      expect(Updates.showMessage).toHaveBeenCalledWith('The UI is disabled in the configuration file. See README.', undefined, 'warning', 'fa-exclamation-triangle');
    });

    it('stores the TYPE and SID cookies to the document', async () => {
      ajaxSpy.and.callFake(() => {
        return Promise.resolve(mockConfig);
      });

      await GerberaApp.initialize();

      expect(GerberaApp.getType()).toEqual('db');
      expect(GerberaApp.isTypeDb()).toBeTruthy();
    });

    it('defaults the TYPE to `db` when none is set', async () => {
      ajaxSpy.and.callFake(() => {
        return Promise.resolve(mockConfig);
      });
      cookieSpy.and.callThrough();

      await GerberaApp.initialize();
      expect(GerberaApp.getType()).toEqual('db');
      expect(GerberaApp.isTypeDb()).toBeTruthy();
    });

    it('initializes all GERBERA components when logged in', async () => {
      ajaxSpy.and.callFake(() => {
        return Promise.resolve(mockConfig);
      });
      cookieSpy.and.callThrough();
      spyOn(Items, 'initialize');
      spyOn(Tree, 'initialize');
      spyOn(Trail, 'initialize');
      spyOn(Autoscan, 'initialize');

      await GerberaApp.initialize();
      expect(Items.initialize).toHaveBeenCalled();
      expect(Tree.initialize).toHaveBeenCalled();
      expect(Trail.initialize).toHaveBeenCalled();
      expect(Autoscan.initialize).toHaveBeenCalled();
      expect(Updates.initialize).toHaveBeenCalled();
    });

    it('sets up Cache-Control headers for all AJAX requests', async () => {
      ajaxSpy.and.callFake(() => {
        return Promise.resolve(mockConfig);
      });

      await GerberaApp.initialize();

      expect(ajaxSetupSpy).toHaveBeenCalledWith({
        beforeSend: jasmine.any(Function)
      });
    });
  });

  describe('disable()', () => {
    it('will disable the menu and clear the content', () => {
      spyOn(Menu, 'disable');
      spyOn(Menu, 'hideLogin');
      spyOn(Menu, 'hideMenu');
      spyOn(Tree, 'destroy');
      spyOn(Trail, 'destroy');
      spyOn(Items, 'destroy');

      GerberaApp.disable();

      expect(Menu.disable).toHaveBeenCalled();
      expect(Menu.hideLogin).toHaveBeenCalled();
      expect(Menu.hideMenu).toHaveBeenCalled();
      expect(Tree.destroy).toHaveBeenCalled();
      expect(Trail.destroy).toHaveBeenCalled();
      expect(Items.destroy).toHaveBeenCalled();
    });
  });

  describe('error()', () => {
    it('shows the event as the error message when event is a string', () => {
      spyOn(Updates, 'showMessage');
      const event = 'Error Message';

      GerberaApp.error(event);

      expect(Updates.showMessage).toHaveBeenCalledWith('Error Message', undefined, 'warning', 'fa-exclamation-triangle');
    });
    it('shows the event as error message when event has responseText', () => {
      spyOn(Updates, 'showMessage');
      const event = {responseText : 'Response Text Error'};
      GerberaApp.error(event);

      expect(Updates.showMessage).toHaveBeenCalledWith('Response Text Error', undefined, 'info', 'fa-exclamation-triangle');
    });
    it('shows the event error taken from the response error', () => {
      spyOn(Updates, 'showMessage');

      GerberaApp.error(uiDisabled);

      expect(Updates.showMessage).toHaveBeenCalledWith('The UI is disabled in the configuration file. See README.', undefined, 'warning', 'fa-exclamation-triangle');
    });
    it('shows unspecified error when event is invalid', () => {
      spyOn(Updates, 'showMessage');
      const event = undefined;

      GerberaApp.error(event);

      expect(Updates.showMessage).toHaveBeenCalledWith('The system encountered an error', undefined, 'danger', 'fa-frown-o');
    });
  });

  describe('isLoggedIn()', () => {
    beforeEach(() => {
      GerberaApp.setLoggedIn(false);
    });
    afterEach(() => {
      GerberaApp.setLoggedIn(false);
    });
    it('returns FALSE by default', () => {
      expect(GerberaApp.isLoggedIn()).toBeFalsy();
    });
    it('allows to be set to TRUE', () => {
      GerberaApp.setLoggedIn(true);
      expect(GerberaApp.isLoggedIn()).toBeTruthy();
    });
  });

  describe('viewItems()', () => {
    it('returns the default value from server config', () => {
      GerberaApp.serverConfig = convertedConfig.config;

      const result = GerberaApp.viewItems();

      expect(result).toEqual(50);
    });
    it('loads the # of view items based on default(25) when app config is undefined', async () => {
      GerberaApp.serverConfig = {};
      expect(GerberaApp.viewItems()).toBe(25);
    });
  });

  describe('currentTreeItem()', () => {
    beforeEach(() => {
      GerberaApp.currentTreeItem = undefined;
    });
    it('returns the default value of undefined', () => {
      const result = GerberaApp.currentTreeItem;
      expect(result).toBeUndefined();
    });
    it('allows to be set with object data', () => {
      GerberaApp.currentTreeItem = {test: 'test'};
      expect(GerberaApp.currentTreeItem).toEqual({test: 'test'});
    });
  });

  describe('setCurrentPage()', () => {
    beforeEach(() => {
      GerberaApp.pageInfo = {};
    });
    it('sets the page number', () => {
      GerberaApp.setCurrentPage(1);
      expect(GerberaApp.currentPage()).toBe(1);
    });
  });
});
