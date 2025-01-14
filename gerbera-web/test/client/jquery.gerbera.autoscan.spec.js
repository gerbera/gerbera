import autoScanItem from './fixtures/autoscan-item';

describe('The jQuery Gerbera Autoscan Overlay', () => {
  'use strict';
  let item;
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
  let autoScanModal;

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
    item = autoScanItem;
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
    autoScanModal = $('#autoscanModal');

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

  describe('loadItem()', () => {
    it('loads the autoscan item into the fields', () => {
      autoScanModal.autoscanmodal('loadItem', {item: item});

      autoscanMode = $('input[name=autoscanMode][value=timed]');
      expect(autoscanId.val()).toBe('2f6d');
      expect(autoscanFromFs.is(':checked')).toBeTruthy();
      expect(autoscanMode.is(':checked')).toBeTruthy();
      expect(autoscanPersistent.is(':checked')).toBeFalsy();
      expect(autoscanRecursive.is(':checked')).toBeTruthy();
      expect(autoscanHidden.is(':checked')).toBeTruthy();
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
    });

    it('marks fields read-only if autoscan is peristent', () => {
      item.persistent = true;
      autoScanModal.autoscanmodal('loadItem', {item: item});

      autoscanMode = $(':radio[value=' + item.scan_mode + ']');
      expect(autoscanMode.is(':disabled')).toBeTruthy();
      expect(autoscanRecursive.is(':disabled')).toBeTruthy();
      expect(autoscanHidden.is(':disabled')).toBeTruthy();
      expect(autoscanFollowSymlinks.is(':checked')).toBeFalsy();
      expect(autoscanInterval.is(':disabled')).toBeTruthy();
      expect(autoscanPersistentMsg.css('display')).toBe('block');
      expect(autoscanSave.is(':disabled')).toBeTruthy();

      expect(autoscanAudio.is(':disabled')).toBeTruthy();
      expect(autoscanAudioMusic.is(':disabled')).toBeTruthy();
      expect(autoscanAudioBook.is(':disabled')).toBeTruthy();
      expect(autoscanAudioBroadcast.is(':disabled')).toBeTruthy();
      expect(autoscanImage.is(':disabled')).toBeTruthy();
      expect(autoscanImagePhoto.is(':disabled')).toBeTruthy();
      expect(autoscanVideo.is(':disabled')).toBeTruthy();
      expect(autoscanVideoMovie.is(':disabled')).toBeTruthy();
      expect(autoscanVideoTV.is(':disabled')).toBeTruthy();
      expect(autoscanVideoMusicVideo.is(':disabled')).toBeTruthy();
      item.persistent = false;
    });

    it('binds the onSave method to the save button', () => {
      const saveSpy = jasmine.createSpy('save');
      autoScanModal.autoscanmodal('loadItem', {item: item, onSave: saveSpy});

      $('#autoscanSave').click();

      expect(saveSpy).toHaveBeenCalled();
    });

    it('shows proper fields for TIMED scan type', () => {
      const previousMode = item.scan_mode;
      item.scan_mode = 'timed';
      autoScanModal.autoscanmodal('loadItem', {item: item});

      expect(autoscanMode.is(':disabled')).toBeFalsy();
      expect(autoscanRecursive.is(':disabled')).toBeFalsy();
      expect(autoscanHidden.is(':disabled')).toBeFalsy();
      expect(autoscanFollowSymlinks.is(':checked')).toBeFalsy();
      expect(autoscanInterval.is(':disabled')).toBeFalsy();

      expect(autoscanAudio.is(':disabled')).toBeFalsy();
      expect(autoscanAudioMusic.is(':disabled')).toBeFalsy();
      expect(autoscanAudioBook.is(':disabled')).toBeFalsy();
      expect(autoscanAudioBroadcast.is(':disabled')).toBeFalsy();
      expect(autoscanImage.is(':disabled')).toBeFalsy();
      expect(autoscanImagePhoto.is(':disabled')).toBeFalsy();
      expect(autoscanVideo.is(':disabled')).toBeFalsy();
      expect(autoscanVideoMovie.is(':disabled')).toBeFalsy();
      expect(autoscanVideoTV.is(':disabled')).toBeFalsy();
      expect(autoscanVideoMusicVideo.is(':disabled')).toBeFalsy();
      item.scan_mode = previousMode;
    });

    it('disables proper fields for NONE scan type', () => {
      const previousMode = item.scan_mode;
      item.scan_mode = 'none';
      autoScanModal.autoscanmodal('loadItem', {item: item});

      expect(autoscanMode.is(':disabled')).toBeFalsy();
      expect(autoscanRecursive.is(':disabled')).toBeTruthy();
      expect(autoscanHidden.is(':disabled')).toBeTruthy();
      expect(autoscanFollowSymlinks.is(':checked')).toBeFalsy();
      expect(autoscanInterval.is(':disabled')).toBeTruthy();

      expect(autoscanAudio.is(':disabled')).toBeTruthy();
      expect(autoscanAudioMusic.is(':disabled')).toBeTruthy();
      expect(autoscanAudioBook.is(':disabled')).toBeTruthy();
      expect(autoscanAudioBroadcast.is(':disabled')).toBeTruthy();
      expect(autoscanImage.is(':disabled')).toBeTruthy();
      expect(autoscanImagePhoto.is(':disabled')).toBeTruthy();
      expect(autoscanVideo.is(':disabled')).toBeTruthy();
      expect(autoscanVideoMovie.is(':disabled')).toBeTruthy();
      expect(autoscanVideoTV.is(':disabled')).toBeTruthy();
      expect(autoscanVideoMusicVideo.is(':disabled')).toBeTruthy();
      item.scan_mode = previousMode;
    });

    it('hides inapplicable fields for INOTIFY type and shows proper labels', () => {
      const previousMode = item.scan_mode;
      item.scan_mode = 'inotify';
      autoScanModal.autoscanmodal('loadItem', {item: item});

      expect(autoscanMode.is(':disabled')).toBeFalsy();
      expect(autoscanRecursive.is(':disabled')).toBeFalsy();
      expect(autoscanHidden.is(':disabled')).toBeFalsy();
      expect(autoscanFollowSymlinks.is(':checked')).toBeFalsy();
      expect(autoscanInterval.is(':disabled')).toBeTruthy();

      expect(autoscanAudio.is(':disabled')).toBeFalsy();
      expect(autoscanAudioMusic.is(':disabled')).toBeFalsy();
      expect(autoscanAudioBook.is(':disabled')).toBeFalsy();
      expect(autoscanAudioBroadcast.is(':disabled')).toBeFalsy();
      expect(autoscanImage.is(':disabled')).toBeFalsy();
      expect(autoscanImagePhoto.is(':disabled')).toBeFalsy();
      expect(autoscanVideo.is(':disabled')).toBeFalsy();
      expect(autoscanVideoMovie.is(':disabled')).toBeFalsy();
      expect(autoscanVideoTV.is(':disabled')).toBeFalsy();
      expect(autoscanVideoMusicVideo.is(':disabled')).toBeFalsy();
      item.scan_mode = previousMode;
    });
  });

  describe('reset()', () => {
    it('resets all fields to empty', () => {
      autoScanModal.autoscanmodal('loadItem', {item: item});
      autoscanMode = $(':radio[id=autoscanModeNone]');

      autoScanModal.autoscanmodal('reset');

      expect(autoscanId.val()).toBe('');
      expect(autoscanFromFs.is(':checked')).toBeFalsy();
      expect(autoscanMode.is(':checked')).toBeFalsy();
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

  describe('saveItem()', () => {
    it('gives proper data for `TIMED` scan mode to save', () => {
      const previousMode = item.scan_mode;
      item.scan_mode = 'timed';
      autoScanModal.autoscanmodal('loadItem', {item: item});

      const result = autoScanModal.autoscanmodal('saveItem');

      expect(result).toEqual({
        object_id: '2f6d',
        from_fs: true,
        scan_mode: 'timed',
        recursive: true,
        hidden: true,
        followSymlinks: false,
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
        ctAudio: '',
        ctImage: '',
        ctVideo: '',
        interval: '1800',
      });
      item.scan_mode = previousMode;
    });

    it('gives proper data for `INOTIFY` scan mode to save', () => {
      const previousMode = item.scan_mode;
      item.scan_mode = 'inotify';
      autoScanModal.autoscanmodal('loadItem', {item: item});

      const result = autoScanModal.autoscanmodal('saveItem');

      expect(result).toEqual({
        object_id: '2f6d',
        from_fs: true,
        scan_mode: 'inotify',
        recursive: true,
        hidden: true,
        followSymlinks: false,
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
        ctAudio: '',
        ctImage: '',
        ctVideo: '',
      });

      item.scan_mode = previousMode;
    });

    it('gives proper data for `NONE` scan mode to save', () => {
      const previousMode = item.scan_mode;
      item.scan_mode = 'none';
      autoScanModal.autoscanmodal('loadItem', {item: item});

      const result = autoScanModal.autoscanmodal('saveItem');

      expect(result).toEqual({
        object_id: '2f6d',
        from_fs: true,
        scan_mode: 'none'
      });

      item.scan_mode = previousMode;
    });
  });
});
