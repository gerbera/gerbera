/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.autoscan.js - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

(function ($) {
  const loadItem = function (modal, itemData) {
    const item = itemData.item;
    const autoscanId = modal.find('#autoscanId');
    const autoscanIdTxt = modal.find('#autoscanIdTxt');
    const autoscanFromFs = modal.find('#autoscanFromFs');
    const autoscanMode = modal.find('input[name=autoscanMode]');
    const autoscanModeNone = modal.find('#autoscanModeNone');
    const autoscanModeInotify = modal.find('#autoscanModeInotify');
    const autoscanModeTimed = modal.find('#autoscanModeTimed');
    const autoscanPersistent = modal.find('#autoscanPersistent');
    const autoscanRecursive = modal.find('#autoscanRecursive');
    const autoscanHidden = modal.find('#autoscanHidden');
    const autoscanSymlinks = modal.find('#autoscanSymlinks');
    const autoscanInterval = modal.find('#autoscanInterval');
    const autoscanSave = modal.find('#autoscanSave');
    const autoscanPersistentMsg = modal.find('#autoscan-persistent-msg');
    const autoscanRetryCount = modal.find('#autoscanRetryCount');
    const autoscanDirTypes = modal.find('#autoscanDirTypes');
    const autoscanForceRescan = modal.find('#autoscanForceRescan');

    const autoscanAudio = modal.find('#autoscanAudio');
    const autoscanAudioMusic = modal.find('#autoscanAudioMusic');
    const autoscanAudioBook = modal.find('#autoscanAudioBook');
    const autoscanAudioBroadcast = modal.find('#autoscanAudioBroadcast');
    const autoscanImage = modal.find('#autoscanImage');
    const autoscanImagePhoto = modal.find('#autoscanImagePhoto');
    const autoscanVideo = modal.find('#autoscanVideo');
    const autoscanVideoMovie = modal.find('#autoscanVideoMovie');
    const autoscanVideoTV = modal.find('#autoscanVideoTV');
    const autoscanVideoMusicVideo = modal.find('#autoscanVideoMusicVideo');

    const autoscanCtAudio = modal.find('#autoscanContainerTypeAudio');
    const autoscanCtImage = modal.find('#autoscanContainerTypeImage');
    const autoscanCtVideo = modal.find('#autoscanContainerTypeVideo');

    reset(modal);
    if (item) {
      if (item.persistent) {
        modal.find('form :input').each(function () {
          $(this).prop('disabled', true)
        });
        autoscanSave.prop('disabled', true).off('click');
        autoscanModeNone.closest('.form-check').hide();
        if (item.scan_mode === 'inotify') {
          autoscanModeTimed.closest('.form-check').hide();
          autoscanModeInotify.closest('.form-check').show();
        } else {
          autoscanModeTimed.closest('.form-check').show();
          autoscanModeInotify.closest('.form-check').hide();
        }
        autoscanPersistentMsg.show();
      } else {
        autoscanSave.off('click').on('click', itemData.onSave);
        autoscanModeNone.closest('.form-check').show();
        autoscanModeTimed.closest('.form-check').show();
        autoscanModeInotify.closest('.form-check').show();
      }

      autoscanId.val(item.object_id);
      autoscanIdTxt.text(item.object_id);
      autoscanIdTxt.prop('title', item.object_id);
      autoscanFromFs.prop('checked', item.from_fs);
      autoscanMode.val([item.scan_mode]);
      autoscanPersistent.prop('checked', item.persistent);
      autoscanRecursive.prop('checked', item.recursive);
      autoscanSymlinks.prop('checked', item.followSymlinks);
      autoscanDirTypes.prop('checked', item.dirTypes);
      autoscanForceRescan.prop('checked', item.forceRescan);
      autoscanHidden.prop('checked', item.hidden);
      autoscanAudio.prop('checked', item.audio);
      autoscanAudioMusic.prop('checked', item.audioMusic);
      autoscanAudioBook.prop('checked', item.audioBook);
      autoscanAudioBroadcast.prop('checked', item.audioBroadcast);
      autoscanImage.prop('checked', item.image);
      autoscanImagePhoto.prop('checked', item.imagePhoto);
      autoscanVideo.prop('checked', item.video);
      autoscanVideoMovie.prop('checked', item.videoMovie);
      autoscanVideoTV.prop('checked', item.videoTV);
      autoscanVideoMusicVideo.prop('checked', item.videoMusicVideo);
      autoscanRetryCount.val(item.retryCount);
      autoscanInterval.val(item.interval);
      autoscanCtAudio.val(item.ctAudio);
      autoscanCtImage.val(item.ctImage);
      autoscanCtVideo.val(item.ctVideo);

      autoscanMode.off('click').on('click', function () {
        adjustFieldApplicability(modal);
      });

      modal.find('#detailAutoscanButton').off('click').on('click', itemData.onDetails);
      modal.find('#hideAutoscanButton').off('click').on('click', itemData.onHide);

      adjustFieldApplicability(modal);
    }
  };

  const adjustFieldApplicability = function (modal) {
    const autoscanMode = modal.find('input[name=autoscanMode]:checked');
    const autoscanRecursive = modal.find('#autoscanRecursive');
    const autoscanHidden = modal.find('#autoscanHidden');
    const autoscanSymlinks = modal.find('#autoscanSymlinks');
    const autoscanInterval = modal.find('#autoscanInterval');
    const autoscanPersistent = modal.find('#autoscanPersistent');
    const autoscanRetryCount = modal.find('#autoscanRetryCount');
    const autoscanDirTypes = modal.find('#autoscanDirTypes');
    const autoscanForceRescan = modal.find('#autoscanForceRescan');

    const autoscanAudio = modal.find('#autoscanAudio');
    const autoscanAudioMusic = modal.find('#autoscanAudioMusic');
    const autoscanAudioBook = modal.find('#autoscanAudioBook');
    const autoscanAudioBroadcast = modal.find('#autoscanAudioBroadcast');
    const autoscanImage = modal.find('#autoscanImage');
    const autoscanImagePhoto = modal.find('#autoscanImagePhoto');
    const autoscanVideo = modal.find('#autoscanVideo');
    const autoscanVideoMovie = modal.find('#autoscanVideoMovie');
    const autoscanVideoTV = modal.find('#autoscanVideoTV');
    const autoscanVideoMusicVideo = modal.find('#autoscanVideoMusicVideo');
    const mediaTypeItems = [autoscanAudio, autoscanAudioMusic, autoscanAudioBook, autoscanAudioBroadcast, autoscanImage, autoscanImagePhoto, autoscanVideo, autoscanVideoMovie, autoscanVideoTV, autoscanVideoMusicVideo];

    const autoscanCtAudio = modal.find('#autoscanContainerTypeAudio');
    const autoscanCtImage = modal.find('#autoscanContainerTypeImage');
    const autoscanCtVideo = modal.find('#autoscanContainerTypeVideo');

    switch (autoscanMode.val()) {
      case 'timed':
        autoscanRecursive.closest('.form-group').removeClass('disabled').show();
        autoscanRecursive.prop('disabled', false);
        autoscanHidden.closest('.form-group').removeClass('disabled').show();
        autoscanHidden.prop('disabled', false);
        autoscanSymlinks.closest('.form-group').removeClass('disabled').show();
        autoscanSymlinks.prop('disabled', false);
        autoscanDirTypes.closest('.form-group').addClass('disabled').show();
        autoscanDirTypes.prop('disabled', false);
        autoscanForceRescan.closest('.form-group').addClass('disabled').show();
        autoscanForceRescan.prop('disabled', false);
        autoscanRetryCount.closest('.form-group').addClass('disabled').show();
        autoscanRetryCount.prop('disabled', false);
        autoscanInterval.closest('.form-group').removeClass('disabled').show();
        autoscanInterval.prop('disabled', false);
        mediaTypeItems.forEach((m) => {
          m.closest('.form-group').removeClass('disabled').show();
          m.prop('disabled', false);
        });
        autoscanCtAudio.closest('.form-group').removeClass('disabled').show();
        autoscanCtAudio.prop('disabled', false);
        autoscanCtImage.closest('.form-group').removeClass('disabled').show();
        autoscanCtImage.prop('disabled', false);
        autoscanCtVideo.closest('.form-group').removeClass('disabled').show();
        autoscanCtVideo.prop('disabled', false);
        if ($("#detailAutoscanCol").is(":hidden"))
          modal.find('#detailAutoscanButton').show();
        break;
      case 'inotify':
        autoscanRecursive.closest('.form-group').removeClass('disabled').show();
        autoscanRecursive.prop('disabled', false);
        autoscanHidden.closest('.form-group').removeClass('disabled').show();
        autoscanHidden.prop('disabled', false);
        autoscanSymlinks.closest('.form-group').removeClass('disabled').show();
        autoscanSymlinks.prop('disabled', false);
        autoscanDirTypes.closest('.form-group').addClass('disabled').show();
        autoscanDirTypes.prop('disabled', false);
        autoscanForceRescan.closest('.form-group').addClass('disabled').show();
        autoscanForceRescan.prop('disabled', false);
        autoscanRetryCount.closest('.form-group').addClass('disabled').show();
        autoscanRetryCount.prop('disabled', false);
        autoscanInterval.closest('.form-group').hide();
        autoscanInterval.prop('disabled', true);
        mediaTypeItems.forEach((m) => {
          m.closest('.form-group').removeClass('disabled').show();
          m.prop('disabled', false);
        });
        autoscanCtAudio.closest('.form-group').removeClass('disabled').show();
        autoscanCtAudio.prop('disabled', false);
        autoscanCtImage.closest('.form-group').removeClass('disabled').show();
        autoscanCtImage.prop('disabled', false);
        autoscanCtVideo.closest('.form-group').removeClass('disabled').show();
        autoscanCtVideo.prop('disabled', false);
        if ($("#detailAutoscanCol").is(":hidden"))
          modal.find('#detailAutoscanButton').show();
        break;
      case 'none':
        autoscanRecursive.closest('.form-group').addClass('disabled').hide();
        autoscanRecursive.prop('disabled', true);
        autoscanHidden.closest('.form-group').addClass('disabled').hide();
        autoscanHidden.prop('disabled', true);
        autoscanSymlinks.closest('.form-group').addClass('disabled').hide();
        autoscanSymlinks.prop('disabled', true);
        autoscanDirTypes.closest('.form-group').addClass('disabled').hide();
        autoscanDirTypes.prop('disabled', true);
        autoscanForceRescan.closest('.form-group').addClass('disabled').hide();
        autoscanForceRescan.prop('disabled', true);
        autoscanRetryCount.closest('.form-group').addClass('disabled').hide();
        autoscanRetryCount.prop('disabled', true);
        autoscanInterval.closest('.form-group').addClass('disabled').hide();
        autoscanInterval.prop('disabled', true);
        mediaTypeItems.forEach((m) => {
          m.closest('.form-group').addClass('disabled').hide();
          m.prop('disabled', true);
        });
        autoscanCtAudio.closest('.form-group').addClass('disabled').hide();
        autoscanCtAudio.prop('disabled', true);
        autoscanCtImage.closest('.form-group').addClass('disabled').hide();
        autoscanCtImage.prop('disabled', true);
        autoscanCtVideo.closest('.form-group').addClass('disabled').hide();
        autoscanCtVideo.prop('disabled', true);
        hideDetails(modal);
        modal.find('#detailAutoscanButton').hide();
        break;
    }

    if (autoscanPersistent.is(':checked')) {
      modal.find('form :input').each(function () {
        $(this).prop('disabled', true);
      });
    }
  };

  const reset = function (modal) {
    const autoscanId = modal.find('#autoscanId');
    const autoscanIdTxt = modal.find('#autoscanIdTxt');
    const autoscanFromFs = modal.find('#autoscanFromFs');
    const autoscanMode = modal.find('input[name=autoscanMode]');
    const autoscanPersistent = modal.find('#autoscanPersistent');
    const autoscanRecursive = modal.find('#autoscanRecursive');
    const autoscanHidden = modal.find('#autoscanHidden');
    const autoscanSymlinks = modal.find('#autoscanSymlinks');
    const autoscanInterval = modal.find('#autoscanInterval');
    const autoscanRetryCount = modal.find('#autoscanRetryCount');
    const autoscanDirTypes = modal.find('#autoscanDirTypes');
    const autoscanForceRescan = modal.find('#autoscanForceRescan');

    const autoscanAudio = modal.find('#autoscanAudio');
    const autoscanAudioMusic = modal.find('#autoscanAudioMusic');
    const autoscanAudioBook = modal.find('#autoscanAudioBook');
    const autoscanAudioBroadcast = modal.find('#autoscanAudioBroadcast');
    const autoscanImage = modal.find('#autoscanImage');
    const autoscanImagePhoto = modal.find('#autoscanImagePhoto');
    const autoscanVideo = modal.find('#autoscanVideo');
    const autoscanVideoMovie = modal.find('#autoscanVideoMovie');
    const autoscanVideoTV = modal.find('#autoscanVideoTV');
    const autoscanVideoMusicVideo = modal.find('#autoscanVideoMusicVideo');
    const mediaTypeItems = [autoscanAudio, autoscanAudioMusic, autoscanAudioBook, autoscanAudioBroadcast, autoscanImage, autoscanImagePhoto, autoscanVideo, autoscanVideoMovie, autoscanVideoTV, autoscanVideoMusicVideo];

    const autoscanCtAudio = modal.find('#autoscanContainerTypeAudio');
    const autoscanCtImage = modal.find('#autoscanContainerTypeImage');
    const autoscanCtVideo = modal.find('#autoscanContainerTypeVideo');

    const autoscanSave = modal.find('#autoscanSave');
    const autoscanPersistentMsg = modal.find('#autoscan-persistent-msg');

    modal.find('form :input').each(function () {
      $(this).prop('disabled', false);
    });

    hideDetails(modal);

    autoscanId.val('');
    autoscanIdTxt.text('');
    autoscanFromFs.prop('checked', false);
    autoscanMode.val(['']);
    autoscanPersistent.prop('checked', false);
    autoscanRecursive.prop('checked', false);
    autoscanSymlinks.prop('checked', false);
    autoscanHidden.prop('checked', false);
    autoscanDirTypes.prop('checked', false);
    autoscanForceRescan.prop('checked', false);
    mediaTypeItems.forEach((m) => {
      m.prop('checked', false);
    });
    autoscanInterval.val('');
    autoscanRetryCount.val('');
    autoscanCtAudio.val('object.container.album.musicAlbum');
    autoscanCtImage.val('object.container.album.photoAlbum');
    autoscanCtVideo.val('object.container');
    autoscanSave.prop('disabled', false);
    autoscanPersistentMsg.hide();
  };

  function showDetails(modal) {
    modal.find('.modal-dialog').addClass('modal-xl');
    $("#detailAutoscanCol").show();
    modal.find('#detailAutoscanButton').hide();
    modal.find('#hideAutoscanButton').show();
  }

  function hideDetails(modal) {
    modal.find('.modal-dialog').removeClass('modal-xl');
    modal.find('#detailAutoscanButton').show();
    modal.find('#hideAutoscanButton').hide();
    $("#detailAutoscanCol").hide();
  }

  function saveItem(modal) {
    const autoscanId = modal.find('#autoscanId');
    const fromFs = modal.find('#autoscanFromFs');
    const autoscanMode = modal.find('input:radio[name=autoscanMode]:checked');
    const autoscanRecursive = modal.find('#autoscanRecursive');
    const autoscanHidden = modal.find('#autoscanHidden');
    const autoscanSymlinks = modal.find('#autoscanSymlinks');
    const autoscanInterval = modal.find('#autoscanInterval');
    const autoscanRetryCount = modal.find('#autoscanRetryCount');
    const autoscanDirTypes = modal.find('#autoscanDirTypes');
    const autoscanForceRescan = modal.find('#autoscanForceRescan');

    const autoscanAudio = modal.find('#autoscanAudio');
    const autoscanAudioMusic = modal.find('#autoscanAudioMusic');
    const autoscanAudioBook = modal.find('#autoscanAudioBook');
    const autoscanAudioBroadcast = modal.find('#autoscanAudioBroadcast');
    const autoscanImage = modal.find('#autoscanImage');
    const autoscanImagePhoto = modal.find('#autoscanImagePhoto');
    const autoscanVideo = modal.find('#autoscanVideo');
    const autoscanVideoMovie = modal.find('#autoscanVideoMovie');
    const autoscanVideoTV = modal.find('#autoscanVideoTV');
    const autoscanVideoMusicVideo = modal.find('#autoscanVideoMusicVideo');

    const autoscanCtAudio = modal.find('#autoscanContainerTypeAudio');
    const autoscanCtImage = modal.find('#autoscanContainerTypeImage');
    const autoscanCtVideo = modal.find('#autoscanContainerTypeVideo');

    let item = {
      object_id: autoscanId.val(),
      from_fs: fromFs.is(':checked'),
      scan_mode: autoscanMode.val()
    };

    switch (autoscanMode.val()) {
      case 'timed':
        item = $.extend({}, item, {
          recursive: autoscanRecursive.is(':checked'),
          hidden: autoscanHidden.is(':checked'),
          followSymlinks: autoscanSymlinks.is(':checked'),
          interval: autoscanInterval.val(),
          retryCount: autoscanRetryCount.val(),
          dirTypes: autoscanDirTypes.is(':checked'),
          forceRescan: autoscanForceRescan.is(':checked'),

          audio: autoscanAudio.is(':checked'),
          audioMusic: autoscanAudioMusic.is(':checked'),
          audioBook: autoscanAudioBook.is(':checked'),
          audioBroadcast: autoscanAudioBroadcast.is(':checked'),
          image: autoscanImage.is(':checked'),
          imagePhoto: autoscanImagePhoto.is(':checked'),
          video: autoscanVideo.is(':checked'),
          videoMovie: autoscanVideoMovie.is(':checked'),
          videoTV: autoscanVideoTV.is(':checked'),
          videoMusicVideo: autoscanVideoMusicVideo.is(':checked'),

          ctAudio: autoscanCtAudio.val(),
          ctImage: autoscanCtImage.val(),
          ctVideo: autoscanCtVideo.val()
        });
        break;
      case 'inotify':
        item = $.extend({}, item, {
          recursive: autoscanRecursive.is(':checked'),
          hidden: autoscanHidden.is(':checked'),
          followSymlinks: autoscanSymlinks.is(':checked'),
          retryCount: autoscanRetryCount.val(),
          dirTypes: autoscanDirTypes.is(':checked'),
          forceRescan: autoscanForceRescan.is(':checked'),

          audio: autoscanAudio.is(':checked'),
          audioMusic: autoscanAudioMusic.is(':checked'),
          audioBook: autoscanAudioBook.is(':checked'),
          audioBroadcast: autoscanAudioBroadcast.is(':checked'),
          image: autoscanImage.is(':checked'),
          imagePhoto: autoscanImagePhoto.is(':checked'),
          video: autoscanVideo.is(':checked'),
          videoMovie: autoscanVideoMovie.is(':checked'),
          videoTV: autoscanVideoTV.is(':checked'),
          videoMusicVideo: autoscanVideoMusicVideo.is(':checked'),

          ctAudio: autoscanCtAudio.val(),
          ctImage: autoscanCtImage.val(),
          ctVideo: autoscanCtVideo.val()
        });
        break;
      case 'none':
        break;
    }

    return item;
  }

  let _super = $.fn.modal;

  // create a new constructor
  let GerberaAutoscanModal = function (element, options) { // eslint-disable-line
    _super.Constructor.apply(this, arguments);
  };

  GerberaAutoscanModal.prototype = $.extend({}, _super.Constructor.prototype, {
    constructor: GerberaAutoscanModal,
    _super: function () {
      let args = $.makeArray(arguments);
      _super.Constructor.prototype[args.shift()].apply(this, args);
    },
    loadItem: function (itemData) {
      return loadItem($(this._element), itemData);
    },
    showDetails: function () {
      return showDetails($(this._element));
    },
    hideDetails: function () {
      return hideDetails($(this._element));
    },
    reset: function () {
      return reset($(this._element));
    },
    saveItem: function () {
      return saveItem($(this._element));
    }
  });

  // override the old initialization with the new constructor
  $.fn.autoscanmodal = $.extend(function (option) {
    let args = $.makeArray(arguments);
    let retval = null;
    option = args.shift();

    this.each(function () {
      let $this = $(this);
      let data = $this.data('modal');
      let options = $.extend({}, _super.defaults, $this.data(), typeof option === 'object' && option);

      if (!data) {
        $this.data('modal', (data = new GerberaAutoscanModal(this, options)));
      }
      if (typeof option === 'string') {
        // for custom methods return the method value, not the selector
        retval = data[option].apply(data, args);
      } else if (options.show) {
        data.show.apply(data, args);
      }
    });

    if (!retval) {
      retval = this;
    }
    return retval;
  }, $.fn.autoscanmodal);
})(jQuery);
