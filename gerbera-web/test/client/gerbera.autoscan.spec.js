import {Autoscan} from '../../../web/js/gerbera-autoscan.module';
import {Auth} from '../../../web/js/gerbera-auth.module';
import {GerberaApp} from '../../../web/js/gerbera-app.module';
import {Updates} from '../../../web/js/gerbera-updates.module';
import item from './fixtures/autoscan-item';
import autoscanResponse from './fixtures/autoscan-add-response';
import submitCompleteResponse from './fixtures/submit-complete-2f6d';

describe('Gerbera Autoscan', () => {
  let lsSpy;
  beforeEach(() => {
    lsSpy = spyOn(window.localStorage, 'getItem').and.callFake((name) => {
        return;
    });
  });

  describe('initialize()', () => {
    let autoscanId;
    let autoscanFromFs;
    let autoscanMode;
    let autoscanPersistent;
    let autoscanRecursive;
    let autoscanHidden;
    let autoscanFollowSymlinks;
    let autoscanInterval;
    let autoscanPersistentMsg;
    let autoscanSave;

    let autoscanAudio;
    let autoscanAudioMusic;
    let autoscanAudioBook;
    let autoscanAudioBroadcast;
    let autoscanImage;
    let autoscanImagePhoto;
    let autoscanVideo;
    let autoscanVideoMovie;
    let autoscanVideoTV;
    let autoscanVideoMusicVideo;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      autoscanId = $('#autoscanId');
      autoscanMode = $('input[name=autoscanMode]');
      autoscanFromFs = $('#autoscanFromFs');
      autoscanPersistent = $('#autoscanPersistent');
      autoscanRecursive = $('#autoscanRecursive');
      autoscanHidden = $('#autoscanHidden');
      autoscanFollowSymlinks = $('#autoscanSymlinks');
      autoscanInterval = $('#autoscanInterval');
      autoscanPersistentMsg = $('#autoscan-persistent-msg');
      autoscanSave = $('#autoscanSave');

      autoscanAudio = $('#autoscanAudio');
      autoscanAudioMusic = $('#autoscanAudioMusic');
      autoscanAudioBook = $('#autoscanAudioBook');
      autoscanAudioBroadcast = $('#autoscanAudioBroadcast');
      autoscanImage = $('#autoscanImage');
      autoscanImagePhoto = $('#autoscanImagePhoto');
      autoscanVideo = $('#autoscanVideo');
      autoscanVideoMovie = $('#autoscanVideoMovie');
      autoscanVideoTV = $('#autoscanVideoTV');
      autoscanVideoMusicVideo = $('#autoscanVideoMusicVideo');
    });
    afterEach(() => {
      fixture.cleanup();
      $('#autoscanModal').remove();
    });
    it('clears all fields in the autoscan modal', () => {
      $('#autoscanModal').autoscanmodal('loadItem', {item: item});

      Autoscan.initialize();

      expect(autoscanId.val()).toBe('');
      expect(autoscanFromFs.is(':checked')).toBeFalsy();
      expect(autoscanMode.val()).toBe('none');
      expect(autoscanPersistent.is(':checked')).toBeFalsy();
      expect(autoscanRecursive.is(':checked')).toBeFalsy();
      expect(autoscanHidden.is(':checked')).toBeFalsy();
      expect(autoscanFollowSymlinks.is(':checked')).toBeFalsy();
      expect(autoscanInterval.val()).toBe('');
      expect(autoscanPersistentMsg.css('display')).toBe('none');
      expect(autoscanSave.is(':disabled')).toBeFalsy();

      expect(autoscanAudio.is(':checked')).toBeFalsy();
      expect(autoscanAudioMusic.is(':checked')).toBeFalsy();
      expect(autoscanAudioBook.is(':checked')).toBeFalsy();
      expect(autoscanAudioBroadcast.is(':checked')).toBeFalsy();
      expect(autoscanImage.is(':checked')).toBeFalsy();
      expect(autoscanImagePhoto.is(':checked')).toBeFalsy();
      expect(autoscanVideo.is(':checked')).toBeFalsy();
      expect(autoscanVideoMovie.is(':checked')).toBeFalsy();
      expect(autoscanVideoTV.is(':checked')).toBeFalsy();
      expect(autoscanVideoMusicVideo.is(':checked')).toBeFalsy();
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
        data: {
          id: '26fd6',
          retryCount: '',
          dirTypes: false,
          forceRescan: false,
          audio: true,
          audioMusic: true,
          audioBook: true,
          audioBroadcast: true,
          image: true,
          imagePhoto: true,
          video: true,
          videoMovie: true,
          videoTV: true,
          videoMusicVideo: true,
          ctAudio: 'object.container.audioMusic',
          ctImage: 'object.container.photoAlbum',
          ctVideo: 'object.container.videoContainer',
        }
      };
      spyOn(Auth, 'getSessionId').and.returnValue('SESSION_ID');
    });

    afterEach(() => {
      fixture.cleanup();
      ajaxSpy.and.callThrough();
      $('#autoscanModal').remove();
    });

    it('calls the server to obtain autoscan edit load', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('fs');
      const data = {
        req_type: 'autoscan',
        action: 'as_edit_load',
        object_id: '26fd6',
        interval: undefined,
        retryCount: '',
        dirTypes: false,
        forceRescan: false,
        audio: true,
        audioMusic: true,
        audioBook: true,
        audioBroadcast: true,
        image: true,
        imagePhoto: true,
        video: true,
        videoMovie: true,
        videoTV: true,
        videoMusicVideo: true,
        from_fs: true,
        ctAudio: 'object.container.audioMusic',
        ctImage: 'object.container.photoAlbum',
        ctVideo: 'object.container.videoContainer',
      };
      data[Auth.SID] = 'SESSION_ID';

      Autoscan.addAutoscan(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });

    it('when calling from DB checks for updates', () => {
      spyOn(GerberaApp, 'getType').and.returnValue('db');
      const data = {
        req_type: 'autoscan',
        action: 'as_edit_load',
        object_id: '26fd6',
        from_fs: false,
        retryCount: '',
        dirTypes: false,
        forceRescan: false,
        audio: true,
        audioMusic: true,
        audioBook: true,
        audioBroadcast: true,
        image: true,
        imagePhoto: true,
        video: true,
        videoMovie: true,
        videoTV: true,
        videoMusicVideo: true,
        updates: 'check',
        ctAudio: 'object.container.audioMusic',
        ctImage: 'object.container.photoAlbum',
        ctVideo: 'object.container.videoContainer',
      };
      data[Auth.SID] = 'SESSION_ID';

      Autoscan.addAutoscan(event);

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
    });
  });
  describe('loadNewAutoscan()', () => {
    let autoscanId;
    let autoscanFromFs;
    let autoscanMode;
    let autoscanPersistent;
    let autoscanRecursive;
    let autoscanHidden;
    let autoscanFollowSymlinks;
    let autoscanInterval;
    let autoscanPersistentMsg;
    let autoscanSave;

    let autoscanAudio;
    let autoscanAudioMusic;
    let autoscanAudioBook;
    let autoscanAudioBroadcast;
    let autoscanImage;
    let autoscanImagePhoto;
    let autoscanVideo;
    let autoscanVideoMovie;
    let autoscanVideoTV;
    let autoscanVideoMusicVideo;

    beforeEach(() => {
      fixture.setBase('test/client/fixtures');
      fixture.load('index.html');
      autoscanId = $('#autoscanId');
      autoscanFromFs = $('#autoscanFromFs');
      autoscanMode = $('input[name=autoscanMode]');
      autoscanPersistent = $('#autoscanPersistent');
      autoscanRecursive = $('#autoscanRecursive');
      autoscanHidden = $('#autoscanHidden');
      autoscanFollowSymlinks = $('#autoscanSymlinks');
      autoscanInterval = $('#autoscanInterval');
      autoscanPersistentMsg = $('#autoscan-persistent-msg');
      autoscanSave = $('#autoscanSave');

      autoscanAudio = $('#autoscanAudio');
      autoscanAudioMusic = $('#autoscanAudioMusic');
      autoscanAudioBook = $('#autoscanAudioBook');
      autoscanAudioBroadcast = $('#autoscanAudioBroadcast');
      autoscanImage = $('#autoscanImage');
      autoscanImagePhoto = $('#autoscanImagePhoto');
      autoscanVideo = $('#autoscanVideo');
      autoscanVideoMovie = $('#autoscanVideoMovie');
      autoscanVideoTV = $('#autoscanVideoTV');
      autoscanVideoMusicVideo = $('#autoscanVideoMusicVideo');
    });
    afterEach(() => {
      fixture.cleanup();
      $('#autoscanModal').remove();
    });

    it('using the response loads the autoscan overlay', () => {
      spyOn(Updates, 'updateTreeByIds');

      Autoscan.loadNewAutoscan(autoscanResponse);

      autoscanMode = $('input[name=autoscanMode][value=timed]');
      expect(autoscanId.val()).toBe('2f6d6');
      expect(autoscanFromFs.is(':checked')).toBeTruthy();
      expect(autoscanMode.is(':checked')).toBeTruthy();
      expect(autoscanPersistent.is(':checked')).toBeFalsy();
      expect(autoscanRecursive.is(':checked')).toBeTruthy();
      expect(autoscanHidden.is(':checked')).toBeFalsy();
      expect(autoscanFollowSymlinks.is(':checked')).toBeFalsy();
      expect(autoscanInterval.val()).toBe('1800');
      expect(autoscanPersistentMsg.css('display')).toBe('none');
      expect(autoscanSave.is(':disabled')).toBeFalsy();

      expect(autoscanAudio.is(':checked')).toBeTruthy();
      expect(autoscanAudioMusic.is(':checked')).toBeTruthy();
      expect(autoscanAudioBook.is(':checked')).toBeTruthy();
      expect(autoscanAudioBroadcast.is(':checked')).toBeTruthy();
      expect(autoscanImage.is(':checked')).toBeTruthy();
      expect(autoscanImagePhoto.is(':checked')).toBeTruthy();
      expect(autoscanVideo.is(':checked')).toBeTruthy();
      expect(autoscanVideoMovie.is(':checked')).toBeTruthy();
      expect(autoscanVideoTV.is(':checked')).toBeTruthy();
      expect(autoscanVideoMusicVideo.is(':checked')).toBeTruthy();

      expect(Updates.updateTreeByIds).toHaveBeenCalled();
    });
  });
  describe('submit()', () => {
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
    });

    afterEach(() => {
      fixture.cleanup();
      $('#autoscanModal').remove();
      ajaxSpy.and.callThrough();
    });

    it('collects all the form data from the autoscan modal to call the server', () => {
      spyOn(Updates, 'updateTreeByIds');
      spyOn(Updates, 'addUiTimer');

      Autoscan.loadNewAutoscan(autoscanResponse);

      Autoscan.submitAutoscan();
      var data = {
        req_type: 'autoscan',
        action: 'as_edit_save',
        object_id: '2f6d6',
        from_fs: true,
        scan_mode: 'timed',
        recursive: true,
        hidden: false,
        retryCount: '',
        dirTypes: false,
        forceRescan: false,
        followSymlinks: false,
        audio: true,
        audioMusic: true,
        audioBook: true,
        audioBroadcast: true,
        image: true,
        imagePhoto: true,
        video: true,
        videoMovie: true,
        videoTV: true,
        videoMusicVideo: true,
        interval: '1800',
        ctAudio: '',
        ctImage: '',
        ctVideo: '',
      };
      data[Auth.SID] = 'SESSION_ID';

      expect(ajaxSpy.calls.count()).toBe(1);
      expect(ajaxSpy.calls.mostRecent().args[0].data).toEqual(data);
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
      Autoscan.initialize();
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'getUpdates');

      Autoscan.submitComplete(submitCompleteResponse);

      expect(Updates.showMessage).toHaveBeenCalledWith('Scan: /Movies', undefined, 'success', 'fa-check');
    });

    it('when successful for one-time scan reports message to user', () => {
      Autoscan.initialize();
      spyOn(Updates, 'showMessage');
      spyOn(Updates, 'getUpdates');

      Autoscan.submitComplete({
        success: true
      });

      expect(Updates.showMessage).toHaveBeenCalledWith('Successfully submitted autoscan', undefined, 'success', 'fa-check');
    });

    it('when fails reports to application error', () => {
      Autoscan.initialize();
      submitCompleteResponse.success = false;
      spyOn(GerberaApp, 'error');

      Autoscan.submitComplete(submitCompleteResponse);

      expect(GerberaApp.error).toHaveBeenCalledWith('Failed to submit autoscan');
      submitCompleteResponse.success = true;
    });
  });
});
