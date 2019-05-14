import item from './fixtures/autoscan-item';
import autoscanResponse from './fixtures/autoscan-add-response';
import submitCompleteResponse from './fixtures/submit-complete-2f6d';

describe('Gerbera Autoscan', () => {
  'use strict';
  describe('initialize()', () => {
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
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
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
    afterEach(() => {
      fixture.cleanup();
      $('#autoscanModal').remove();
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
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      event = {
        data: { id: '26fd6' }
      };
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
    });

    afterEach(() => {
      fixture.cleanup();
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
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
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

    afterEach((done) => {
      $("body").on('transitionend', function(event){
        fixture.cleanup();
        $('#autoscanModal').remove();
        done();
      });
    });

    it('using the response loads the autoscan overlay', () => {
      spyOn(GERBERA.Updates, 'updateTreeByIds');

      GERBERA.Autoscan.loadNewAutoscan(autoscanResponse);

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
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      spyOn(GERBERA.Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(GERBERA.Updates, 'showMessage');
      spyOn(GERBERA.Updates, 'getUpdates');
    });

    afterEach((done) => {
      $("body").on('transitionend', function(event){
        fixture.cleanup();
        $('#autoscanModal').remove();
        ajaxSpy.and.callThrough();
        done();
      });
    });

    it('collects all the form data from the autoscan modal to call the server', () => {
      spyOn(GERBERA.Updates, 'updateTreeByIds');
      spyOn(GERBERA.Updates, 'addUiTimer');

      GERBERA.Autoscan.loadNewAutoscan(autoscanResponse);

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

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
    });

    afterEach(() => {
      fixture.cleanup();
    });

    it('when successful reports a message to the user and starts task interval', () => {
      GERBERA.Autoscan.initialize();
      spyOn(GERBERA.Updates, 'showMessage');
      spyOn(GERBERA.Updates, 'getUpdates');

      GERBERA.Autoscan.submitComplete(submitCompleteResponse);

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
      submitCompleteResponse.success = false;
      spyOn(GERBERA.App, 'error');

      GERBERA.Autoscan.submitComplete(submitCompleteResponse);

      expect(GERBERA.App.error).toHaveBeenCalledWith('Failed to submit autoscan');
      submitCompleteResponse.success = true;
    });
  });
});
