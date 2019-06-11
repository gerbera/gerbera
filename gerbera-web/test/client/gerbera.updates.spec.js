import {Updates} from "../../../web/js/gerbera-updates.module";
import {Auth} from "../../../web/js/gerbera-auth.module";
import {GerberaApp} from "../../../web/js/gerbera-app.module";
import {Tree} from '../../../web/js/gerbera-tree.module';
import updatesNoTaskId from './fixtures/updates-no-taskId';
import updatesWithTaskId from './fixtures/updates-with-task';
import updatesWithNoTask from './fixtures/updates-with-no-task';
import updatesWithPendingUpdates from './fixtures/updates-with-pending-updates';
import updatesWithNoUiUpdates from './fixtures/updates-with-no-ui-updates';
import updatesWithNoPendingUpdates from './fixtures/updates-with-no-pending-updates';
import invalidResponse from './fixtures/invalid-response';
import uiDisabled from './fixtures/ui-disabled';
import sessionExpired from './fixtures/session-expired';
import updateIds from './fixtures/update_ids';

describe('Gerbera Updates', function () {
  'use strict';

  beforeEach(function () {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    Updates.initialize();
  });

  afterEach(() => {
    fixture.cleanup();
  });
  describe('showMessage()', function () {
    it('uses toast to display a message to the user', function () {
      spyOn($.fn, 'toast');

      Updates.showMessage('a message');

      expect($.fn.toast).toHaveBeenCalledWith('show', {message: 'a message', type: undefined, icon: undefined});
    });
  });
  describe('getUpdates()', function () {
    let ajaxSpy;

    beforeEach(function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function () {
        return Promise.resolve({});
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server check for updates', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);
      spyOn(GerberaApp, 'getType').and.returnValue('db');

      spyOn(Updates, 'updateTask').and.callFake(() => {
        return Promise.resolve({});
      });

      await Updates.getUpdates();

      expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface');
      expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
        req_type: 'void',
        sid: 'SESSION_ID',
        updates: 'check'
      });
    });

    it('calls the server get for updates when forced', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      spyOn(Updates, 'updateTask').and.callFake(() => {
        return Promise.resolve({});
      });

      const force = true;
      await Updates.getUpdates(force);

      expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface');
      expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
        req_type: 'void',
        sid: 'SESSION_ID',
        updates: 'get'
      });
    });

    it('updates the current task when response from server', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);

      spyOn(Updates, 'updateTask').and.callFake(() => {
        return Promise.resolve({});
      });

      await Updates.getUpdates();

      expect(Updates.updateTask).toHaveBeenCalled();
    });

    it('updates the timer to call back to the server', async () => {
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);
      spyOn(Updates, 'updateTask').and.callFake(() => {
        return Promise.resolve({});
      });
      spyOn(Updates, 'updateUi').and.callFake(() => {
        return Promise.resolve({});
      });

      await Updates.getUpdates();

      expect(Updates.updateUi).toHaveBeenCalled();
    });

    it('clears the timer if the call to server fails', async () => {
      ajaxSpy.calls.reset();
      ajaxSpy.and.callFake(() => {
        return $.Deferred().reject();
      });

      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GerberaApp, 'isLoggedIn').and.returnValue(true);
      spyOn(Updates, 'updateTask');
      spyOn(Updates, 'updateUi');
      spyOn(Updates, 'clearAll').and.callFake(() => {
        return Promise.resolve({});
      });

      try {
        await Updates.getUpdates();
      } catch (err) {
        expect(Updates.updateTask).not.toHaveBeenCalled();
        expect(Updates.updateUi).not.toHaveBeenCalled();
        expect(Updates.clearAll).toHaveBeenCalled();
      }
    });
  });
  describe('updateTask()', () => {

    it('clears the polling interval when task ID is negative', async () => {
      spyOn(Updates, 'clearTaskInterval').and.returnValue(Promise.resolve(updatesNoTaskId));

      const promisedResponse = await Updates.updateTask(updatesNoTaskId);

      expect(Updates.clearTaskInterval).toHaveBeenCalled();
      expect(promisedResponse).toEqual(updatesNoTaskId);
    });

    it('updates the task information if task exists', async () => {
      spyOn(Updates, 'addTaskInterval');

      const promisedResponse = await Updates.updateTask(updatesWithTaskId);

      expect($('#grb-toast-msg').text()).toEqual('Performing full scan: /Movies');
      expect(promisedResponse).toEqual(updatesWithTaskId);
    });

    it('creates a polling interval when tasks still exist', async () => {
      spyOn(Updates, 'addTaskInterval');

      const promisedResponse = await Updates.updateTask(updatesWithTaskId);

      expect(Updates.addTaskInterval).toHaveBeenCalled();
      expect(promisedResponse).toEqual(updatesWithTaskId);
    });

    it('passes the response onto the next method', async () => {
      spyOn(Updates, 'addTaskInterval');

      const promisedResponse = await Updates.updateTask(updatesWithTaskId);

      expect(promisedResponse).toEqual(updatesWithTaskId);
    });

    it('passes the response onto the next method when no task exists', async () => {
      const promisedResponse = await Updates.updateTask(updatesWithNoTask);

      expect(promisedResponse).toEqual(updatesWithNoTask);
    });
  });
  describe('updateUi()', () => {

    it('when pending UI updates, sets a timeout to be called later', async () => {
      spyOn(Updates, 'addUiTimer').and.returnValue(Promise.resolve(updatesWithPendingUpdates));

      const promisedResponse = await Updates.updateUi(updatesWithPendingUpdates);

      expect(Updates.addUiTimer).toHaveBeenCalled();
      expect(promisedResponse).toEqual(updatesWithPendingUpdates);
    });

    it('when no UI updates, clears the UI timeout', async () => {
      spyOn(Updates, 'clearUiTimer').and.returnValue(Promise.resolve(updatesWithNoUiUpdates));

      const promisedResponse = await Updates.updateUi(updatesWithNoUiUpdates);

      expect(Updates.clearUiTimer).toHaveBeenCalled();
      expect(promisedResponse).toEqual(updatesWithNoUiUpdates);
    });

    it('when no pending updates, clears the UI timeout', async () => {
      spyOn(Updates, 'clearUiTimer').and.returnValue(Promise.resolve(updatesWithNoPendingUpdates));

      const promisedResponse = await Updates.updateUi(updatesWithNoPendingUpdates);

      expect(Updates.clearUiTimer).toHaveBeenCalled();
      expect(promisedResponse).toEqual(updatesWithNoPendingUpdates);
    });
  });
  describe('errorCheck()', () => {
    let uiDisabledResponse;
    let sessionExpiredResponse;
    let event;
    let xhr;
    let toastMsg;

    beforeEach(() => {
      toastMsg = $('#grb-toast-msg');
      toastMsg.text('');
    });

    it('shows a toast message when AJAX error returns failure', () => {
      event = {};
      xhr = {
        responseJSON: invalidResponse
      };

      Updates.errorCheck(event, xhr);

      expect(toastMsg.text()).toEqual('General Error');
    });

    it('ignores when response does not exist', () => {
      event = {};
      xhr = {
        responseJSON: {}
      };

      Updates.errorCheck(event, xhr);

      expect(toastMsg.text()).toEqual('');
    });

    it('disables application when server returns 900 error code, albeit successful', () => {
      spyOn(GerberaApp, 'disable');
      event = {};
      xhr = {
        responseJSON: uiDisabled
      };

      Updates.errorCheck(event, xhr);

      expect(toastMsg.text()).toEqual('The UI is disabled in the configuration file. See README.');
      expect(GerberaApp.disable).toHaveBeenCalled();
    });

    it('clears session cookie and redirects to home when server returns 400 error code session invalid.', () => {
      spyOn(Auth, 'handleLogout');
      event = {};
      xhr = {
        responseJSON: sessionExpired
      };

      Updates.errorCheck(event, xhr);

      expect(Auth.handleLogout).toHaveBeenCalled();
    });
  });
  describe('addUiTimer()', () => {
    let updateSpy;

    beforeAll(function () {
      updateSpy = spyOn(Updates, 'getUpdates');
    });

    beforeEach(() => {
      updateSpy.calls.reset();
      Updates.clearAll();
    });

    afterAll(() => {
      Updates.clearAll();
    });

    it('uses the default interval to set a timeout', (done) => {
      GerberaApp.serverConfig = {};
      GerberaApp.serverConfig['poll-interval'] = 100;
      const startTime = new Date().getTime();
      updateSpy.and.callFake(() => {
        const currentTime = new Date().getTime();
        expect((currentTime - startTime) >= 95).toBeTruthy('Actual value was ' + (currentTime - startTime));
        expect((currentTime - startTime) <= 200).toBeTruthy('Actual value was ' + (currentTime - startTime));
        Updates.clearAll();
        done();
      });

      Updates.addUiTimer();
    });

    it('overrides the default when passed in', (done) => {
      GerberaApp.serverConfig = {};
      GerberaApp.serverConfig['poll-interval'] = 200;
      const startTime = new Date().getTime();
      updateSpy.and.callFake(() => {
        const currentTime = new Date().getTime();
        expect((currentTime - startTime) >= 195).toBeTruthy('Actual value was ' + (currentTime - startTime));
        expect((currentTime - startTime) <= 300).toBeTruthy('Actual value was ' + (currentTime - startTime));
        Updates.clearAll();
        done();
      });

      Updates.addUiTimer();
    });
  });
  describe('addTaskInterval()', () => {
    let updateSpy;
    beforeAll(function () {
      updateSpy = spyOn(Updates, 'getUpdates');
    });

    beforeEach(() => {
      updateSpy.calls.reset();
      Updates.clearAll();
    });

    it('sets a recurring interval based on the poll-interval configuration', (done) => {
      GerberaApp.serverConfig = {};
      GerberaApp.serverConfig['poll-interval'] = 100;
      const startTime = new Date().getTime();
      let totalCount = 0;
      updateSpy.and.callFake(() => {
        totalCount++;
        let currentTime = new Date().getTime();
        if (totalCount >= 2) {
          expect((currentTime - startTime) >= 195).toBeTruthy('Interval should accumulate time from start but was ' + (currentTime - startTime));
          expect((currentTime - startTime) <= 210).toBeTruthy('Interval should accumulate time from start but was ' + (currentTime - startTime));
          Updates.clearAll();
          done();
        }
      });

      Updates.addTaskInterval();
    });
  });
  describe('updateTreeByIds()', () => {
    let response;

    it('given a failed response does nothing', () => {
      spyOn(Tree, 'reloadTreeItemById');
      response = {
        success: false,
        update_ids: {
          updates: true,
          ids: 'all'
        }
      };

      Updates.updateTreeByIds(response);

      expect(Tree.reloadTreeItemById).not.toHaveBeenCalledWith();
    });

    it('given a response with `all` update ID reloads the whole tree', () => {
      spyOn(Tree, 'reloadTreeItemById');
      response = {
        success: true,
        update_ids: {
          updates: true,
          ids: 'all'
        }
      };
      Updates.updateTreeByIds(response);

      expect(Tree.reloadTreeItemById).toHaveBeenCalledWith('0');
    });

    it('given a response with 1 update ID reloads the parent item of the id', () => {
      spyOn(Tree, 'reloadParentTreeItem');

      Updates.updateTreeByIds(updateIds);

      expect(Tree.reloadParentTreeItem).toHaveBeenCalledWith('8');
    });

    it('given a response with several update ID reloads the parent item for each id', () => {
      spyOn(Tree, 'reloadParentTreeItem');
      response = {
        success: true,
        update_ids: {
          updates: true,
          ids: '2,4,5,6'
        }
      };

      Updates.updateTreeByIds(response);

      expect(Tree.reloadParentTreeItem.calls.count()).toBe(4);
    });

    it('clears the update timer when updates are false', () => {
      spyOn(Updates, 'clearUiTimer');
      response = {
        success: true,
        update_ids: {
          updates: false
        }
      };

      Updates.updateTreeByIds(response);

      expect(Updates.clearUiTimer).toHaveBeenCalled();
    });

    it('starts the update timer when updates are pending', () => {
      spyOn(Updates, 'addUiTimer');
      response = {
        success: true,
        update_ids: {
          pending: true
        }
      };

      Updates.updateTreeByIds(response);

      expect(Updates.addUiTimer).toHaveBeenCalled();
    });
  });
});
