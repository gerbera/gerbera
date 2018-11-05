/* global GERBERA jasmine it expect describe beforeEach beforeAll afterAll getJSONFixture loadFixtures loadJSONFixtures spyOn */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('Gerbera Updates', function () {
  'use strict';

  beforeEach(function () {
    loadFixtures('index.html');
    GERBERA.Updates.initialize();
  });

  describe('showMessage()', function () {
    it('uses toast to display a message to the user', function () {
      spyOn($.fn, 'toast');

      GERBERA.Updates.showMessage('a message');

      expect($.fn.toast).toHaveBeenCalledWith('show', {message: 'a message', type: undefined, icon: undefined});
    });
  });

  describe('getUpdates()', function () {
    let ajaxSpy;

    beforeEach(function () {
      ajaxSpy = spyOn($, 'ajax').and.callFake(function () {
        return $.Deferred().resolve({}).promise();
      });
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server check for updates', async () => {
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.App, 'getType').and.returnValue('db');

      spyOn(GERBERA.Updates, 'updateTask').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });

      await GERBERA.Updates.getUpdates();

      expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface');
      expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
        req_type: 'void',
        sid: 'SESSION_ID',
        updates: 'check'
      });
    });

    it('calls the server get for updates when forced', async () => {
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      spyOn(GERBERA.Updates, 'updateTask').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });

      const force = true;
      GERBERA.Updates.getUpdates(force);

      expect(ajaxSpy.calls.mostRecent().args[0]['url']).toEqual('content/interface');
      expect(ajaxSpy.calls.mostRecent().args[0]['data']).toEqual({
        req_type: 'void',
        sid: 'SESSION_ID',
        updates: 'get'
      });
    });

    it('updates the current task when response from server', async () => {
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);

      spyOn(GERBERA.Updates, 'updateTask').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });

      await GERBERA.Updates.getUpdates();

      expect(GERBERA.Updates.updateTask).toHaveBeenCalled();
    });

    it('updates the timer to call back to the server', async () => {
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.Updates, 'updateTask').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      spyOn(GERBERA.Updates, 'updateUi').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });

      await GERBERA.Updates.getUpdates();

      expect(GERBERA.Updates.updateUi).toHaveBeenCalled();
    });

    it('clears the timer if the call to server fails', async () => {
      ajaxSpy.calls.reset();
      ajaxSpy.and.callFake(() => {
        return $.Deferred().reject();
      });

      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Auth, 'isLoggedIn').and.returnValue(true);
      spyOn(GERBERA.Updates, 'updateTask');
      spyOn(GERBERA.Updates, 'updateUi');
      spyOn(GERBERA.Updates, 'clearAll').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });

      try {
        await GERBERA.Updates.getUpdates();
      } catch (err) {
        expect(GERBERA.Updates.updateTask).not.toHaveBeenCalled();
        expect(GERBERA.Updates.updateUi).not.toHaveBeenCalled();
        expect(GERBERA.Updates.clearAll).toHaveBeenCalled();
      }
    });
  });

  describe('updateTask()', () => {
    let response;

    beforeEach(function () {
      loadJSONFixtures('updates-no-taskId.json');
      loadJSONFixtures('updates-with-task.json');
      loadJSONFixtures('updates-with-no-task.json');
    });

    it('clears the polling interval when task ID is negative', async () => {
      response = getJSONFixture('updates-no-taskId.json');
      spyOn(GERBERA.Updates, 'clearTaskInterval').and.returnValue($.Deferred().resolve(response).promise());

      const promisedResponse = await GERBERA.Updates.updateTask(response);

      expect(GERBERA.Updates.clearTaskInterval).toHaveBeenCalled();
      expect(promisedResponse).toEqual(response);
    });

    it('updates the task information if task exists', async () => {
      response = getJSONFixture('updates-with-task.json');
      spyOn(GERBERA.Updates, 'addTaskInterval');

      const promisedResponse = await GERBERA.Updates.updateTask(response);

      expect($('#grb-toast-msg').text()).toEqual('Performing full scan: /Movies');
      expect(promisedResponse).toEqual(response);
    });

    it('creates a polling interval when tasks still exist', async () => {
      response = getJSONFixture('updates-with-task.json');
      spyOn(GERBERA.Updates, 'addTaskInterval');

      const promisedResponse = await GERBERA.Updates.updateTask(response);

      expect(GERBERA.Updates.addTaskInterval).toHaveBeenCalled();
      expect(promisedResponse).toEqual(response);
    });

    it('passes the response onto the next method', async () => {
      response = getJSONFixture('updates-with-task.json');
      spyOn(GERBERA.Updates, 'addTaskInterval');

      const promisedResponse = await GERBERA.Updates.updateTask(response);

      expect(promisedResponse).toEqual(response);
    });

    it('passes the response onto the next method when no task exists', async () => {
      response = getJSONFixture('updates-with-no-task.json');

      const promisedResponse = await GERBERA.Updates.updateTask(response);

      expect(promisedResponse).toEqual(response);
    });
  });

  describe('updateUi()', () => {
    let response;

    beforeEach(function () {
      loadJSONFixtures('updates-with-pending-updates.json');
      loadJSONFixtures('updates-with-no-ui-updates.json');
      loadJSONFixtures('updates-with-no-pending-updates.json');
    });

    it('when pending UI updates, sets a timeout to be called later', async () => {
      response = getJSONFixture('updates-with-pending-updates.json');
      spyOn(GERBERA.Updates, 'addUiTimer').and.returnValue($.Deferred().resolve(response).promise());

      const promisedResponse = await GERBERA.Updates.updateUi(response);

      expect(GERBERA.Updates.addUiTimer).toHaveBeenCalled();
      expect(promisedResponse).toEqual(response);
    });

    it('when no UI updates, clears the UI timeout', async () => {
      response = getJSONFixture('updates-with-no-ui-updates.json');
      spyOn(GERBERA.Updates, 'clearUiTimer').and.returnValue($.Deferred().resolve(response).promise());

      const promisedResponse = await GERBERA.Updates.updateUi(response);

      expect(GERBERA.Updates.clearUiTimer).toHaveBeenCalled();
      expect(promisedResponse).toEqual(response);
    });

    it('when no pending updates, clears the UI timeout', async () => {
      response = getJSONFixture('updates-with-no-pending-updates.json');
      spyOn(GERBERA.Updates, 'clearUiTimer').and.returnValue($.Deferred().resolve(response).promise());

      const promisedResponse = await GERBERA.Updates.updateUi(response);

      expect(GERBERA.Updates.clearUiTimer).toHaveBeenCalled();
      expect(promisedResponse).toEqual(response);
    });
  });

  describe('errorCheck()', () => {
    let response;
    let uiDisabledResponse;
    let sessionExpiredResponse;
    let event;
    let xhr;
    let toastMsg;

    beforeEach(() => {
      loadJSONFixtures('invalid-response.json');
      response = getJSONFixture('invalid-response.json');
      toastMsg = $('#grb-toast-msg');
      toastMsg.text('');
    });

    it('shows a toast message when AJAX error returns failure', () => {
      event = {};
      xhr = {
        responseJSON: response
      };

      GERBERA.Updates.errorCheck(event, xhr);

      expect(toastMsg.text()).toEqual('General Error');
    });

    it('ignores when response does not exist', () => {
      event = {};
      xhr = {
        responseJSON: {}
      };

      GERBERA.Updates.errorCheck(event, xhr);

      expect(toastMsg.text()).toEqual('');
    });

    it('disables application when server returns 900 error code, albeit successful', () => {
      loadJSONFixtures('ui-disabled.json');
      uiDisabledResponse = getJSONFixture('ui-disabled.json');
      spyOn(GERBERA.App, 'disable');
      event = {};
      xhr = {
        responseJSON: uiDisabledResponse
      };

      GERBERA.Updates.errorCheck(event, xhr);

      expect(toastMsg.text()).toEqual('The UI is disabled in the configuration file. See README.');
      expect(GERBERA.App.disable).toHaveBeenCalled();
    });

    it('clears session cookie and redirects to home when server returns 400 error code session invalid.', () => {
      spyOn(GERBERA.Auth, 'handleLogout');
      // when calling for fixtures, ajaxComplete fires `errorCheck`
      // so spies must be before to avoid full page refresh.
      loadJSONFixtures('session-expired.json');
      sessionExpiredResponse = getJSONFixture('session-expired.json');
      event = {};
      xhr = {
        responseJSON: sessionExpiredResponse
      };

      GERBERA.Updates.errorCheck(event, xhr);

      expect(GERBERA.Auth.handleLogout).toHaveBeenCalled();
    });
  });

  describe('addUiTimer()', () => {
    let updateSpy;

    beforeAll(function () {
      updateSpy = spyOn(GERBERA.Updates, 'getUpdates');
    });

    beforeEach(() => {
      updateSpy.calls.reset();
      GERBERA.Updates.clearAll();
    });

    afterAll(() => {
      GERBERA.Updates.clearAll();
    });

    it('uses the default interval to set a timeout', (done) => {
      GERBERA.App.serverConfig = {};
      GERBERA.App.serverConfig['poll-interval'] = 100;
      const startTime = new Date().getTime();
      updateSpy.and.callFake(() => {
        const currentTime = new Date().getTime();
        expect((currentTime - startTime) >= 95).toBeTruthy('Actual value was ' + (currentTime - startTime));
        expect((currentTime - startTime) <= 110).toBeTruthy('Actual value was ' + (currentTime - startTime));
        GERBERA.Updates.clearAll();
        done();
      });

      GERBERA.Updates.addUiTimer();
    });

    it('overrides the default when passed in', (done) => {
      GERBERA.App.serverConfig = {};
      GERBERA.App.serverConfig['poll-interval'] = 200;
      const startTime = new Date().getTime();
      updateSpy.and.callFake(() => {
        const currentTime = new Date().getTime();
        expect((currentTime - startTime) >= 195).toBeTruthy('Actual value was ' + (currentTime - startTime));
        expect((currentTime - startTime) <= 210).toBeTruthy('Actual value was ' + (currentTime - startTime));
        GERBERA.Updates.clearAll();
        done();
      });

      GERBERA.Updates.addUiTimer();
    });
  });

  describe('addTaskInterval()', () => {
    let updateSpy;
    beforeAll(function () {
      updateSpy = spyOn(GERBERA.Updates, 'getUpdates');
    });

    beforeEach(() => {
      updateSpy.calls.reset();
      GERBERA.Updates.clearAll();
    });

    it('sets a recurring interval based on the poll-interval configuration', (done) => {
      GERBERA.App.serverConfig = {};
      GERBERA.App.serverConfig['poll-interval'] = 100;
      const startTime = new Date().getTime();
      let totalCount = 0;
      updateSpy.and.callFake(() => {
        totalCount++;
        let currentTime = new Date().getTime();
        if (totalCount >= 2) {
          expect((currentTime - startTime) >= 195).toBeTruthy('Interval should accumulate time from start but was ' + (currentTime - startTime));
          expect((currentTime - startTime) <= 210).toBeTruthy('Interval should accumulate time from start but was ' + (currentTime - startTime));
          GERBERA.Updates.clearAll();
          done();
        }
      });

      GERBERA.Updates.addTaskInterval();
    });
  });

  describe('updateTreeByIds()', () => {
    let response;

    beforeEach(() => {
      loadJSONFixtures('update_ids.json');
      response = getJSONFixture('update_ids.json');
    });

    it('given a failed response does nothing', () => {
      spyOn(GERBERA.Tree, 'reloadTreeItemById');
      response = {
        success: false,
        update_ids: {
          updates: true,
          ids: 'all'
        }
      };

      GERBERA.Updates.updateTreeByIds(response);

      expect(GERBERA.Tree.reloadTreeItemById).not.toHaveBeenCalledWith();
    });

    it('given a response with `all` update ID reloads the whole tree', () => {
      spyOn(GERBERA.Tree, 'reloadTreeItemById');
      response = {
        success: true,
        update_ids: {
          updates: true,
          ids: 'all'
        }
      };
      GERBERA.Updates.updateTreeByIds(response);

      expect(GERBERA.Tree.reloadTreeItemById).toHaveBeenCalledWith('0');
    });

    it('given a response with 1 update ID reloads the parent item of the id', () => {
      spyOn(GERBERA.Tree, 'reloadParentTreeItem');

      GERBERA.Updates.updateTreeByIds(response);

      expect(GERBERA.Tree.reloadParentTreeItem).toHaveBeenCalledWith('8');
    });

    it('given a response with several update ID reloads the parent item for each id', () => {
      spyOn(GERBERA.Tree, 'reloadParentTreeItem');
      response = {
        success: true,
        update_ids: {
          updates: true,
          ids: '2,4,5,6'
        }
      };

      GERBERA.Updates.updateTreeByIds(response);

      expect(GERBERA.Tree.reloadParentTreeItem.calls.count()).toBe(4);
    });

    it('clears the update timer when updates are false', () => {
      spyOn(GERBERA.Updates, 'clearUiTimer');
      response = {
        success: true,
        update_ids: {
          updates: false
        }
      };

      GERBERA.Updates.updateTreeByIds(response);

      expect(GERBERA.Updates.clearUiTimer).toHaveBeenCalled();
    });

    it('starts the update timer when updates are pending', () => {
      spyOn(GERBERA.Updates, 'addUiTimer');
      response = {
        success: true,
        update_ids: {
          pending: true
        }
      };

      GERBERA.Updates.updateTreeByIds(response);

      expect(GERBERA.Updates.addUiTimer).toHaveBeenCalled();
    });
  });
});
