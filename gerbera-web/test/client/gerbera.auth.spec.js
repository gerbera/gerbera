/* global GERBERA jasmine it expect spyOn describe beforeEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('Gerbera Auth', () => {
  describe('checkSID()', () => {
    let ajaxSpy, mockGetSid;
    beforeEach(() => {
      loadJSONFixtures('get_sid.not.logged_in.json');
      mockGetSid = getJSONFixture('get_sid.not.logged_in.json');

      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve(mockGetSid).promise();
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server for a new SID and sets the cookie for SID', async () => {
      await GERBERA.Auth.checkSID();
      expect($.cookie('SID')).toBe('563806f88aea6b33429ebdb85ce14beb');
      expect(GERBERA.Auth.isLoggedIn()).toBeFalsy();
    });

    it('calls GERBERA.App.error when the server call fails', async () => {
      spyOn(GERBERA.App, 'error');
      ajaxSpy.and.callFake(() => {
        return $.Deferred().reject({}).promise();
      });
      try {
        await GERBERA.Auth.checkSID();
      } catch(err) {
        expect(GERBERA.App.error).toHaveBeenCalled();
      }
    });
  });

  describe('handleLogout()', () => {
    beforeEach(() => {
      $.cookie('SID', '563806f88aea6b33429ebdb85ce14beb');
    });

    it('removes existing SID cookie and routes to the home page', () => {
      spyOn(GERBERA.App, 'reload');

      GERBERA.Auth.handleLogout();

      expect(GERBERA.App.reload).toHaveBeenCalledWith('/index.html');
      expect(GERBERA.Auth.getSessionId()).toBeUndefined();
      expect(GERBERA.Auth.isLoggedIn()).toBeFalsy();
    });
  });

  describe('SID()', () => {
    it('retrieves the session ID from the cookie', () => {
      spyOn($, 'cookie').and.returnValue('A_MOCK_SESSION_ID');
      expect(GERBERA.Auth.getSessionId()).toEqual('A_MOCK_SESSION_ID');
    });
  });

  describe('authenticate()', () => {
    let mockToken, mockLogin, ajaxSpy;
    beforeEach(() => {
      loadJSONFixtures('get_token.success.json');
      mockToken = getJSONFixture('get_token.success.json');
      mockLogin = getJSONFixture('login.success.json');
      loadFixtures('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake((options) => {
        const d = $.Deferred();
        if (options.data.action === 'get_token') {
          d.resolve(mockToken);
        } else if (options.data.action === 'login') {
          d.resolve(mockLogin);
        }
        return d.promise();
      });
      ajaxSpy.calls.reset();
    });

    afterEach(() => {
      ajaxSpy.calls.reset();
      ajaxSpy.and.callThrough();
    });

    it('calls the server to get a token and then logs the user into the system', async () => {
      $('#username').val('user_name');
      $('#password').val('password');
      GERBERA.App.serverConfig = {
        accounts: true
      };
      spyOn(GERBERA.Tree, 'initialize');
      spyOn(GERBERA.Items, 'initialize');
      spyOn(GERBERA.Menu, 'initialize');
      spyOn(GERBERA.Trail, 'initialize');
      spyOn(GERBERA.Autoscan, 'initialize');
      spyOn(GERBERA.Updates, 'initialize');

      await GERBERA.Auth.authenticate();
      expect(GERBERA.Auth.isLoggedIn()).toBeTruthy();
      expect(ajaxSpy.calls.count()).toBe(2);
      expect(GERBERA.Tree.initialize).toHaveBeenCalled();
      expect(GERBERA.Items.initialize).toHaveBeenCalled();
      expect(GERBERA.Menu.initialize).toHaveBeenCalled();
      expect(GERBERA.Trail.initialize).toHaveBeenCalled();
      expect(GERBERA.Autoscan.initialize).toHaveBeenCalled();
      expect(GERBERA.Updates.initialize).toHaveBeenCalled();
    });
  });

  describe('logout()', () => {
    let ajaxSpy;
    beforeEach(() => {
      loadFixtures('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve({success: true}).promise();
      });
      $.cookie('SID', '12345');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to logout the session and expires the session ID cookie', async () => {
      spyOn(GERBERA.App, 'reload');

      await GERBERA.Auth.logout();

      expect(GERBERA.Auth.isLoggedIn()).toBeFalsy();
      expect(GERBERA.Auth.getSessionId()).toBeUndefined();
      expect($('#nav-db').attr('class')).toBe('nav-link disabled');
      expect($('#nav-fs').attr('class')).toBe('nav-link disabled');
      expect(GERBERA.App.reload).toHaveBeenCalledWith('/index.html');
    });
  });
});
