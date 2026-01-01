/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.dirtweak.spec.js - this file is part of Gerbera.

    Copyright (C) 2016-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/
/* global fixture */
import { Tweaks } from '../../../web/js/gerbera-tweak.module';
import { Auth } from '../../../web/js/gerbera-auth.module';
import { GerberaApp } from '../../../web/js/gerbera-app.module';
import { Updates } from '../../../web/js/gerbera-updates.module';
import dirTweakItem from './fixtures/dirtweak-item';
import dirTweakSubmit from './fixtures/dirtweak-submit';
import dirTweakDeleteContent from './fixtures/dirtweak-delete';
import submitCompleteResponse from './fixtures/submit-complete-2f6d';

describe('Gerbera DirTweaks', () => {
  let lsSpy;
  beforeEach(() => {
    lsSpy = spyOn(window.localStorage, 'getItem').and.callFake((name) => {
      return;
    });
  });

  afterEach(() => {
    $('#dirTweakModal').remove();
    fixture.cleanup();
  });

  describe('initialize()', () => {

    let dirTweakLocation;
    let dirTweakId;
    let dirTweakIndex;
    let dirTweakStatus;
    let dirTweakInherit;
    let dirTweakSymLinks;
    let dirTweakRecursive;
    let dirTweakCaseSens;
    let dirTweakHidden;
    let dirTweakMetaCharset;
    let dirTweakFanArt;
    let dirTweakSubtitle;
    let dirTweakResource;
    let dirTweakDelete;
    let dirTweakSave;
    let dirTweakModal;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');

      dirTweakLocation = $('#dirTweakLocation');
      dirTweakId = $('#dirTweakId');
      dirTweakIndex = $('#dirTweakIndex');
      dirTweakStatus = $('#dirTweakStatus');
      dirTweakInherit = $('#dirTweakInherit');
      dirTweakSymLinks = $('#dirTweakSymLinks');
      dirTweakRecursive = $('#dirTweakRecursive');
      dirTweakCaseSens = $('#dirTweakCaseSens');
      dirTweakHidden = $('#dirTweakHidden');
      dirTweakMetaCharset = $('#dirTweakMetaCharset');
      dirTweakFanArt = $('#dirTweakFanArt');
      dirTweakSubtitle = $('#dirTweakSubtitle');
      dirTweakResource = $('#dirTweakResource');
      dirTweakDelete = $('#dirTweakDelete');
      dirTweakSave = $('#dirTweakSave');

      dirTweakModal = $('#dirTweakModal');
    });
    afterEach(() => {
      fixture.cleanup();
      $('#dirTweakModal').remove();
    });
    it('clears all fields in the tweak modal', () => {
      dirTweakModal.dirtweakmodal('loadItem', dirTweakItem);

      Tweaks.initialize();

      expect(dirTweakLocation.val()).toBe('/tmp');
      expect(dirTweakInherit.is(':checked')).toBeFalsy();
      expect(dirTweakSymLinks.is(':checked')).toBeFalsy();
      expect(dirTweakRecursive.is(':checked')).toBeFalsy();
      expect(dirTweakCaseSens.is(':checked')).toBeFalsy();
      expect(dirTweakMetaCharset.val()).toBe('');
      expect(dirTweakFanArt.val()).toBe('');
      expect(dirTweakSubtitle.val()).toBe('');
      expect(dirTweakResource.val()).toBe('');
      expect(dirTweakDelete.prop('hidden')).toBeTruthy();
      expect(dirTweakSave.is(':disabled')).toBeTruthy();
    });
  });

  describe('addDirTweak()', () => {
    let ajaxSpy, event;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return $.Deferred().resolve({}).promise();
      });
      event = {
        data: { fullPath: '/home/gerbera/Music' }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.and.callThrough();
      $('#dirTweakModal').remove();
    });

    it('calls the server to obtain config including directory tweaks', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('fs');
      const data = {
        req_type: 'config_load',
      };
      data[Auth.SID] = 'SESSION_ID';

      Tweaks.addDirTweak(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });
  });

  describe('loadNewDirTweak()', () => {
    let dirTweakLocation;
    let dirTweakId;
    let dirTweakIndex;
    let dirTweakStatus;
    let dirTweakInherit;
    let dirTweakSymLinks;
    let dirTweakRecursive;
    let dirTweakCaseSens;
    let dirTweakHidden;
    let dirTweakMetaCharset;
    let dirTweakFanArt;
    let dirTweakSubtitle;
    let dirTweakResource;
    let dirTweakDelete;
    let dirTweakSave;
    let dirTweakModal;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      Updates.initialize();
      Tweaks.initialize();

      dirTweakLocation = $('#dirTweakLocation');
      dirTweakId = $('#dirTweakId');
      dirTweakIndex = $('#dirTweakIndex');
      dirTweakStatus = $('#dirTweakStatus');
      dirTweakInherit = $('#dirTweakInherit');
      dirTweakSymLinks = $('#dirTweakSymLinks');
      dirTweakRecursive = $('#dirTweakRecursive');
      dirTweakCaseSens = $('#dirTweakCaseSens');
      dirTweakHidden = $('#dirTweakHidden');
      dirTweakMetaCharset = $('#dirTweakMetaCharset');
      dirTweakFanArt = $('#dirTweakFanArt');
      dirTweakSubtitle = $('#dirTweakSubtitle');
      dirTweakResource = $('#dirTweakResource');
      dirTweakDelete = $('#dirTweakDelete');
      dirTweakSave = $('#dirTweakSave');

      dirTweakModal = $('#dirTweakModal');
    });

    afterEach(() => {
      fixture.cleanup();
      $('#dirTweakModal').remove();
    });

    it('using the response loads the dirtweak overlay', () => {
      Tweaks.loadNewDirTweak(dirTweakItem, '/home/gerbera/Music');

      expect(dirTweakLocation.val()).toBe('/home/gerbera/Music');
      expect(dirTweakInherit.is(':checked')).toBeFalsy();
      expect(dirTweakSymLinks.is(':checked')).toBeTruthy();
      expect(dirTweakRecursive.is(':checked')).toBeTruthy();
      expect(dirTweakCaseSens.is(':checked')).toBeFalsy();
      expect(dirTweakMetaCharset.val()).toBe('');
      expect(dirTweakFanArt.val()).toBe('cover.jpg');
      expect(dirTweakSubtitle.val()).toBe('');
      expect(dirTweakResource.val()).toBe('');
      expect(dirTweakDelete.prop('hidden')).toBeFalsy();
      expect(dirTweakSave.is(':disabled')).toBeFalsy();
      dirTweakModal.remove();
    });
  });

  describe('submitDirTweak()', () => {
    let ajaxSpy;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'getUpdates');
      spyOn(Updates, 'clearAll');
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.and.callThrough();
      $('#dirTweakModal').remove();
    });

    it('collects submit form data from the dirtweak modal to call the server', async () => {
      Tweaks.loadNewDirTweak(dirTweakItem, '/home/gerbera/Music');
      $('#dirTweakHidden').prop('checked', false);
      Tweaks.submitDirTweak();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(dirTweakSubmit);
    });
  });

  describe('deleteDirTweak()', () => {
    let ajaxSpy;
    let dirTweakDeleteButton;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      dirTweakDeleteButton = $('#dirTweakDelete');
      ajaxSpy = spyOn($, 'ajax').and.callFake(() => {
        return Promise.resolve({});
      });
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'getUpdates');
      spyOn(Updates, 'clearAll');
    });

    it('collects delete data from the dirtweak modal to call the server', async () => {
      Tweaks.loadNewDirTweak(dirTweakItem, '/home/gerbera/Music');
      expect(dirTweakDeleteButton.prop('hidden')).toBeFalsy();
      await dirTweakDeleteButton.click();

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(dirTweakDeleteContent);
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.and.callThrough();
      $('#dirTweakModal').remove();
    });
  });

  describe('submitDirTweakComplete()', () => {

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
    });

    afterEach(() => {
      fixture.cleanup();
      $('#dirTweakModal').remove();
    });

    it('when successful reports a message to the user and starts task interval', () => {
      Tweaks.initialize();
      Updates.initialize();
      spyOn(Updates, 'showMessage');

      Tweaks.submitDirTweakComplete(submitCompleteResponse);

      expect(Updates.showMessage).toHaveBeenCalledWith('Scan: /Movies', undefined, 'success', 'fa-check');
    });

    it('when successful for full scan reports message to user', () => {
      Tweaks.initialize();
      Updates.initialize();
      spyOn(Updates, 'showMessage');

      Tweaks.submitDirTweakComplete({
        success: true
      });

      expect(Updates.showMessage).toHaveBeenCalledWith('Successfully submitted directory tweak', undefined, 'success', 'fa-check');
    });

    it('when fails reports to application error', () => {
      Tweaks.initialize();
      submitCompleteResponse.success = false;
      spyOn(GerberaApp, 'error');

      Tweaks.submitDirTweakComplete(submitCompleteResponse);

      expect(GerberaApp.error).toHaveBeenCalledWith('Failed to submit directory tweak');
      submitCompleteResponse.success = true;
    });
  });
});
