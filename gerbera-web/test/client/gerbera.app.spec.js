/* global GERBERA jasmine it expect spyOn describe beforeEach beforeAll afterEach loadJSONFixtures getJSONFixture */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('Gerbera UI App', () => {
  describe('initialize()', () => {
    let mockConfig, convertedConfig, ajaxSpy, cookieSpy, uiDisabled, ajaxSetupSpy;

    beforeAll(() => {
      loadJSONFixtures('converted-config.json');
      loadJSONFixtures('config.json');
      loadJSONFixtures('ui-disabled.json');
      mockConfig = getJSONFixture('config.json');
      convertedConfig = getJSONFixture('converted-config.json');
      uiDisabled = getJSONFixture('ui-disabled.json');
      ajaxSpy = spyOn($, 'ajax');
      ajaxSetupSpy = spyOn($, 'ajaxSetup');
      spyOn(GERBERA.Auth, 'checkSID');
    });

    beforeEach(() => {
      cookieSpy = spyOn($, 'cookie').and.callFake((name) => {
        if (name === 'TYPE') return 'db';
        if (name === 'SID') return 'A_MOCK_SID';
        return 'A_MOCK_COOKIE';
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
      ajaxSetupSpy.and.callThrough();
    });

    it('retrieves the configuration from the server using AJAX', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(mockConfig).promise();
      });
      await GERBERA.App.initialize();

      expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface');
      expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
        req_type: 'auth',
        sid: 'A_MOCK_SID',
        action: 'get_config'
      });

      expect(GERBERA.App.serverConfig).toEqual(convertedConfig.config);
      expect(GERBERA.Auth.checkSID).toHaveBeenCalled();
      expect($(document).attr('title')).toBe('Gerbera Media Server | Gerbera Media Server');
    });

    it('reports an error when the ajax calls fails', async () => {
      spyOn(GERBERA.Updates, 'showMessage');

      ajaxSpy.and.callFake(() => {
        return $.Deferred().reject({responseText: 'Internal Server Error'});
      });

      try {
        await GERBERA.App.initialize();
      } catch (err) {
        expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Internal Server Error', undefined, 'info', 'fa-exclamation-triangle');
      }
    });

    it('reports an error when UI is disabled', async () => {
      spyOn(GERBERA.Updates, 'showMessage');
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(uiDisabled).promise();
      });

      try {
        await GERBERA.App.initialize();
      } catch (err) {
        expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('The UI is disabled in the configuration file. See README.', undefined, 'warning', 'fa-exclamation-triangle');
      }
    });

    it('stores the TYPE and SID cookies to the document', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(mockConfig).promise()
      });

      await GERBERA.App.initialize();
      expect(GERBERA.App.getType()).toEqual('db');
      expect(GERBERA.App.isTypeDb()).toBeTruthy();
      expect(GERBERA.Auth.getSessionId()).toBe('A_MOCK_SID');
    });

    it('defaults the TYPE to `db` when none is set', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(mockConfig).promise();
      });
      cookieSpy.and.callThrough();

      await GERBERA.App.initialize();
      expect(GERBERA.App.getType()).toEqual('db');
      expect(GERBERA.App.isTypeDb()).toBeTruthy();
    });

    it('initializes all GERBERA components when logged in', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(mockConfig).promise();
      });
      cookieSpy.and.callThrough();
      spyOn(GERBERA.Items, 'initialize');
      spyOn(GERBERA.Tree, 'initialize');
      spyOn(GERBERA.Trail, 'initialize');
      spyOn(GERBERA.Autoscan, 'initialize');
      spyOn(GERBERA.Updates, 'initialize');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);

      await GERBERA.App.initialize();
      expect(GERBERA.Items.initialize).toHaveBeenCalled();
      expect(GERBERA.Tree.initialize).toHaveBeenCalled();
      expect(GERBERA.Trail.initialize).toHaveBeenCalled();
      expect(GERBERA.Autoscan.initialize).toHaveBeenCalled();
      expect(GERBERA.Updates.initialize).toHaveBeenCalled();
    });

    it('sets up Cache-Control headers for all AJAX requests', async () => {
      ajaxSpy.and.callFake(() => {
        return $.Deferred().resolve(mockConfig).promise();
      });

      await GERBERA.App.initialize();

      expect(ajaxSetupSpy).toHaveBeenCalledWith({
        beforeSend: jasmine.any(Function)
      });
    });
  });

  describe('disable()', () => {
    it('will disable the menu and clear the content', () => {
      spyOn(GERBERA.Menu, 'disable');
      spyOn(GERBERA.Menu, 'hideLogin');
      spyOn(GERBERA.Menu, 'hideMenu');
      spyOn(GERBERA.Tree, 'destroy');
      spyOn(GERBERA.Trail, 'destroy');
      spyOn(GERBERA.Items, 'destroy');

      GERBERA.App.disable();

      expect(GERBERA.Menu.disable).toHaveBeenCalled();
      expect(GERBERA.Menu.hideLogin).toHaveBeenCalled();
      expect(GERBERA.Menu.hideMenu).toHaveBeenCalled();
      expect(GERBERA.Tree.destroy).toHaveBeenCalled();
      expect(GERBERA.Trail.destroy).toHaveBeenCalled();
      expect(GERBERA.Items.destroy).toHaveBeenCalled();
    });
  });

  describe('error()', () => {
    let uiDisabled;

    beforeEach(() => {
      loadJSONFixtures('ui-disabled.json');
      uiDisabled = getJSONFixture('ui-disabled.json');
    });

    it('shows the event as the error message when event is a string', () => {
      spyOn(GERBERA.Updates, 'showMessage');
      const event = 'Error Message';
      GERBERA.App.error(event);

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Error Message', undefined, 'warning', 'fa-exclamation-triangle');
    });

    it('shows the event as error message when event has responseText', () => {
      spyOn(GERBERA.Updates, 'showMessage');
      const event = {responseText : 'Response Text Error'};
      GERBERA.App.error(event);

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Response Text Error', undefined, 'info', 'fa-exclamation-triangle');
    });

    it('shows the event error taken from the response error', () => {
      spyOn(GERBERA.Updates, 'showMessage');

      GERBERA.App.error(uiDisabled);

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('The UI is disabled in the configuration file. See README.', undefined, 'warning', 'fa-exclamation-triangle');
    });

    it('shows unspecified error when event is invalid', () => {
      spyOn(GERBERA.Updates, 'showMessage');
      const event = undefined;

      GERBERA.App.error(event);

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('The system encountered an error', undefined, 'danger', 'fa-frown-o');
    });
  });
});
