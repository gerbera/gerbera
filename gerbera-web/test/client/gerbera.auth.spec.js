import {Auth} from '../../../web/js/gerbera-auth.module';
import {GerberaApp} from '../../../web/js/gerbera-app.module';
import {Tree} from '../../../web/js/gerbera-tree.module';
import {Items} from '../../../web/js/gerbera-items.module';
import {Menu} from '../../../web/js/gerbera-menu.module';
import {Trail} from '../../../web/js/gerbera-trail.module';
import {Autoscan} from '../../../web/js/gerbera-autoscan.module';
import {Updates} from '../../../web/js/gerbera-updates.module';
import mockGetSid from './fixtures/get_sid.not.logged_in';
import mockToken from './fixtures/get_token.success';
import mockLogin from './fixtures/login.success';

describe('Gerbera Auth', () => {
  describe('checkSID()', () => {
    let ajaxSpy;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve(mockGetSid);
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    describe('checkSID()', () => {
      beforeEach(() => {
        GerberaApp.setLoggedIn(false);
      });

      it('calls the server for a new SID and sets the cookie for SID', async () => {
        await Auth.checkSID();
        const sessionId = Cookies.get('SID');
        expect(sessionId).toBe('563806f88aea6b33429ebdb85ce14beb');
        expect(GerberaApp.isLoggedIn()).toBeFalsy();
      });

      it('calls GERBERA.App.error when the server call fails', async () => {
        spyOn(GerberaApp, 'error');
        ajaxSpy.and.callFake(() => {
          return Promise.reject({});
        });
        await Auth.checkSID();
        expect(GerberaApp.error).toHaveBeenCalled();
      });
    });
  });
  describe('handleLogout()', () => {
    beforeEach(() => {
      Cookies.set('SID', '563806f88aea6b33429ebdb85ce14beb');
      GerberaApp.setLoggedIn(true);
    });

    it('removes existing SID cookie and routes to the home page', () => {
      spyOn(GerberaApp, 'reload');

      Auth.handleLogout();

      expect(GerberaApp.reload).toHaveBeenCalledWith('/index.html');
      expect(Auth.getSessionId()).toBeUndefined();
      expect(GerberaApp.isLoggedIn()).toBeFalsy();
    });
  });
  describe('getSessionId()', () => {
    it('retrieves the session ID from the cookie', () => {
      spyOn(Cookies, 'get').and.returnValue('A_MOCK_SESSION_ID');
      expect(Auth.getSessionId()).toEqual('A_MOCK_SESSION_ID');
    });
  });
  describe('logout()', () => {
    let ajaxSpy;
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({success: true});
      });
      Cookies.set('SID', '12345');
      GerberaApp.setLoggedIn(true);
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.and.callThrough();
    });

    it('calls the server to logout the session and expires the session ID cookie', async () => {
      spyOn(GerberaApp, 'reload');

      await Auth.logout();

      expect(GerberaApp.isLoggedIn()).toBeFalsy();
      expect(Auth.getSessionId()).toBeUndefined();
      expect($('#nav-db').attr('class')).toBe('nav-link disabled');
      expect($('#nav-fs').attr('class')).toBe('nav-link disabled');
      expect(GerberaApp.reload).toHaveBeenCalledWith('/index.html');
    });
  });
  describe('authenticate()', () => {
    let ajaxSpy;
    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake((options) => {
        return new Promise((resolve, reject) => {
          if (options.data.action === 'get_token') {
            resolve(mockToken);
          } else if (options.data.action === 'login') {
            resolve(mockLogin);
          }
        });
      });
      ajaxSpy.calls.reset();
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.calls.reset();
      ajaxSpy.and.callThrough();
    });

    it('calls the server to get a token and then logs the user into the system', async () => {
      $('#username').val('user_name');
      $('#password').val('password');
      GerberaApp.serverConfig = {
        accounts: true
      };
      spyOn(Tree, 'initialize');
      spyOn(Items, 'initialize');
      spyOn(Menu, 'initialize');
      spyOn(Trail, 'initialize');
      spyOn(Autoscan, 'initialize');
      spyOn(Updates, 'initialize');

      await Auth.authenticate();

      expect(GerberaApp.isLoggedIn()).toBeTruthy();
      expect(ajaxSpy.calls.count()).toBe(2);
      expect(Tree.initialize).toHaveBeenCalled();
      expect(Items.initialize).toHaveBeenCalled();
      expect(Menu.initialize).toHaveBeenCalled();
      expect(Trail.initialize).toHaveBeenCalled();
      expect(Autoscan.initialize).toHaveBeenCalled();
      expect(Updates.initialize).toHaveBeenCalled();
    });
  });
});