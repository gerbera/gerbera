/* global GERBERA jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures spyOn */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('Gerbera Autoscan', () => {
  'use strict';
  describe('initialize()', () => {
    let item;
    let autoscanId;
    let autoscanFromFs;
    let autoscanMode;
    let autoscanLevel;
    let autoscanPersistent;
    let autoscanRecursive;
    let autoscanHidden;
    let autoscanInterval;
    let autoscanPersistentMsg;
    let autoscanSave;

    beforeEach(() => {
      loadFixtures('index.html');
      loadJSONFixtures('autoscan-item.json');
      item = getJSONFixture('autoscan-item.json');
      autoscanId = $('#autoscanId');
      autoscanMode = $('input[name=autoscanMode]');
      autoscanLevel = $('input[name=autoscanLevel]');
      autoscanFromFs = $('#autoscanFromFs');
      autoscanPersistent = $('#autoscanPersistent');
      autoscanRecursive = $('#autoscanRecursive');
      autoscanHidden = $('#autoscanHidden');
      autoscanInterval = $('#autoscanInterval');
      autoscanPersistentMsg = $('#autoscan-persistent-msg');
      autoscanSave = $('#autoscanSave');
    });

    it('clears all fields in the autoscan modal', () => {
      $('#autoscanModal').autoscanmodal('loadItem', {item: item});

      GERBERA.Autoscan.initialize();

      expect(autoscanId.val()).toBe('');
      expect(autoscanFromFs.is(':checked')).toBeFalsy();
      expect(autoscanMode.val()).toBe('none');
      expect(autoscanLevel.val()).toBe('basic');
      expect(autoscanPersistent.is(':checked')).toBeFalsy();
      expect(autoscanRecursive.is(':checked')).toBeFalsy();
      expect(autoscanHidden.is(':checked')).toBeFalsy();
      expect(autoscanInterval.val()).toBe('');
      expect(autoscanPersistentMsg.css('display')).toBe('none');
      expect(autoscanSave.is(':disabled')).toBeFalsy();
    });
  });

  describe('addAutoscan()', () => {
    let ajaxSpy, event;

    beforeEach(() => {
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      event = {
        data: { id: '26fd6' }
      };
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('calls the server to obtain autoscan edit load', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('fs');
      const data = {
        req_type: 'autoscan',
        action: 'as_edit_load',
        object_id: '26fd6',
        sid: 'SESSION_ID',
        from_fs: true
      };

      GERBERA.Autoscan.addAutoscan(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('when calling from DB checks for updates', () => {
      spyOn(GERBERA.App, 'getType').and.returnValue('db');
      const data = {
        req_type: 'autoscan',
        action: 'as_edit_load',
        object_id: '26fd6',
        sid: 'SESSION_ID',
        from_fs: false,
        updates: 'check'
      };

      GERBERA.Autoscan.addAutoscan(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });
  });

  describe('loadNewAutoscan()', () => {
    let autoscanId;
    let autoscanFromFs;
    let autoscanMode;
    let autoscanLevel;
    let autoscanPersistent;
    let autoscanRecursive;
    let autoscanHidden;
    let autoscanInterval;
    let autoscanPersistentMsg;
    let autoscanSave;

    beforeEach(() => {
      loadFixtures('index.html');
      loadJSONFixtures('autoscan-add-response.json');
      autoscanId = $('#autoscanId');
      autoscanFromFs = $('#autoscanFromFs');
      autoscanMode = $('input[name=autoscanMode]');
      autoscanLevel = $('input[name=autoscanLevel]');
      autoscanPersistent = $('#autoscanPersistent');
      autoscanRecursive = $('#autoscanRecursive');
      autoscanHidden = $('#autoscanHidden');
      autoscanInterval = $('#autoscanInterval');
      autoscanPersistentMsg = $('#autoscan-persistent-msg');
      autoscanSave = $('#autoscanSave');
    });

    it('using the response loads the autoscan overlay', () => {
      const response = getJSONFixture('autoscan-add-response.json');
      spyOn(GERBERA.Updates, 'updateTreeByIds');

      GERBERA.Autoscan.loadNewAutoscan(response);

      autoscanMode = $('input[name=autoscanMode][value=timed]');
      autoscanLevel = $('input[name=autoscanLevel][value=full]');
      expect(autoscanId.val()).toBe('2f6d6');
      expect(autoscanFromFs.is(':checked')).toBeTruthy();
      expect(autoscanMode.is(':checked')).toBeTruthy();
      expect(autoscanLevel.is(':checked')).toBeTruthy();
      expect(autoscanPersistent.is(':checked')).toBeFalsy();
      expect(autoscanRecursive.is(':checked')).toBeTruthy();
      expect(autoscanHidden.is(':checked')).toBeFalsy();
      expect(autoscanInterval.val()).toBe('1800');
      expect(autoscanPersistentMsg.css('display')).toBe('none');
      expect(autoscanSave.is(':disabled')).toBeFalsy();

      expect(GERBERA.Updates.updateTreeByIds).toHaveBeenCalled();
    });
  });

  describe('submit()', () => {
    let response, ajaxSpy;

    beforeEach(() => {
      loadFixtures('index.html');
      loadJSONFixtures('autoscan-add-response.json');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Updates, 'showMessage');
      spyOn(GERBERA.Updates, 'getUpdates');
    });

    afterEach(() => {
      ajaxSpy.and.callThrough();
    });

    it('collects all the form data from the autoscan modal to call the server', () => {
      response = getJSONFixture('autoscan-add-response.json');
      spyOn(GERBERA.Updates, 'updateTreeByIds');
      spyOn(GERBERA.Updates, 'addUiTimer');

      GERBERA.Autoscan.loadNewAutoscan(response);

      GERBERA.Autoscan.submitAutoscan();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual({
        sid: 'SESSION_ID',
        req_type: 'autoscan',
        action: 'as_edit_save',
        object_id: '2f6d6',
        from_fs: true,
        scan_mode: 'timed',
        scan_level: 'full',
        recursive: true,
        hidden: false,
        interval: '1800'
      });
    });
  });

  describe('submitComplete()', () => {
    let response;
    beforeEach(() => {
      loadFixtures('index.html');
      loadJSONFixtures('submit-complete-2f6d.json');
      response = getJSONFixture('submit-complete-2f6d.json');
    });

    it('when successful reports a message to the user and starts task interval', () => {
      GERBERA.Autoscan.initialize();
      spyOn(GERBERA.Updates, 'showMessage');
      spyOn(GERBERA.Updates, 'getUpdates');

      GERBERA.Autoscan.submitComplete(response);

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Performing full scan: /Movies', undefined, 'success', 'fa-check');
    });

    it('when successful for one-time scan reports message to user', () => {
      GERBERA.Autoscan.initialize();
      spyOn(GERBERA.Updates, 'showMessage');
      spyOn(GERBERA.Updates, 'getUpdates');

      GERBERA.Autoscan.submitComplete({
        success: true
      });

      expect(GERBERA.Updates.showMessage).toHaveBeenCalledWith('Successfully submitted autoscan', undefined, 'success', 'fa-check');
    });

    it('when fails reports to application error', () => {
      GERBERA.Autoscan.initialize();
      response.success = false;
      spyOn(GERBERA.App, 'error');

      GERBERA.Autoscan.submitComplete(response);

      expect(GERBERA.App.error).toHaveBeenCalledWith('Failed to submit autoscan');
      response.success = true;
    });
  });
});
