/* global jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('The jQuery Gerbera Autoscan Overlay', function () {
  'use strict'
  var item
  var autoscanId
  var autoscanFromFs
  var autoscanMode
  var autoscanLevel
  var autoscanPersistent
  var autoscanRecursive
  var autoscanHidden
  var autoscanInterval
  var autoscanPersistentMsg
  var autoscanSave

  beforeEach(function () {
    loadFixtures('autoscan-modal.html')
    loadJSONFixtures('autoscan-item.json')
    item = getJSONFixture('autoscan-item.json')
    autoscanId = $('#autoscanId')
    autoscanFromFs = $('#autoscanFromFs')
    autoscanMode = $('input[name=autoscanMode]')
    autoscanLevel = $('input[name=autoscanLevel]')
    autoscanPersistent = $('#autoscanPersistent')
    autoscanRecursive = $('#autoscanRecursive')
    autoscanHidden = $('#autoscanHidden')
    autoscanInterval = $('#autoscanInterval')
    autoscanPersistentMsg = $('#autoscan-persistent-msg')
    autoscanSave = $('#autoscanSave')
  })

  describe('loadItem()', function () {
    it('loads the autoscan item into the fields', function () {
      $('#autoscanModal').autoscanmodal('loadItem', {item: item})

      autoscanMode = $('input[name=autoscanMode][value=timed]')
      autoscanLevel = $('input[name=autoscanLevel][value=full]')
      expect(autoscanId.val()).toBe('2f6d')
      expect(autoscanFromFs.is(':checked')).toBeTruthy()
      expect(autoscanMode.is(':checked')).toBeTruthy()
      expect(autoscanLevel.is(':checked')).toBeTruthy()
      expect(autoscanPersistent.is(':checked')).toBeFalsy()
      expect(autoscanRecursive.is(':checked')).toBeTruthy()
      expect(autoscanHidden.is(':checked')).toBeTruthy()
      expect(autoscanInterval.val()).toBe('1800')
      expect(autoscanPersistentMsg.css('display')).toBe('none')
      expect(autoscanSave.is(':disabled')).toBeFalsy()
    })

    it('marks fields read-only if autoscan is peristent', function () {
      item.persistent = true
      $('#autoscanModal').autoscanmodal('loadItem', {item: item})

      autoscanMode = $(':radio[value=' + item.scan_mode + ']')
      autoscanLevel = $(':radio[value=' + item.scan_level + ']')
      expect(autoscanMode.is(':disabled')).toBeTruthy()
      expect(autoscanLevel.is(':disabled')).toBeTruthy()
      expect(autoscanRecursive.is(':disabled')).toBeTruthy()
      expect(autoscanHidden.is(':disabled')).toBeTruthy()
      expect(autoscanInterval.is(':disabled')).toBeTruthy()
      expect(autoscanPersistentMsg.css('display')).toBe('block')
      expect(autoscanSave.is(':disabled')).toBeTruthy()
      item.persistent = false
    })

    it('binds the onSave method to the save button', function () {
      var saveSpy = jasmine.createSpy('save')
      $('#autoscanModal').autoscanmodal('loadItem', {item: item, onSave: saveSpy})

      $('#autoscanSave').click()

      expect(saveSpy).toHaveBeenCalled()
    })

    it('shows proper fields for TIMED scan type', function () {
      var previousMode = item.scan_mode
      item.scan_mode = 'timed'
      $('#autoscanModal').autoscanmodal('loadItem', {item: item})

      expect(autoscanMode.is(':disabled')).toBeFalsy()
      expect(autoscanLevel.is(':disabled')).toBeFalsy()
      expect(autoscanRecursive.is(':disabled')).toBeFalsy()
      expect(autoscanHidden.is(':disabled')).toBeFalsy()
      expect(autoscanInterval.is(':disabled')).toBeFalsy()
      item.scan_mode = previousMode
    })

    it('disables proper fields for NONE scan type', function () {
      var previousMode = item.scan_mode
      item.scan_mode = 'none'
      $('#autoscanModal').autoscanmodal('loadItem', {item: item})

      expect(autoscanMode.is(':disabled')).toBeFalsy()
      expect(autoscanLevel.is(':disabled')).toBeTruthy()
      expect(autoscanRecursive.is(':disabled')).toBeTruthy()
      expect(autoscanHidden.is(':disabled')).toBeTruthy()
      expect(autoscanInterval.is(':disabled')).toBeTruthy()
      item.scan_mode = previousMode
    })

    it('hides inapplicable fields for INOTIFY type and shows proper labels', function () {
      var previousMode = item.scan_mode
      item.scan_mode = 'inotify'
      $('#autoscanModal').autoscanmodal('loadItem', {item: item})

      expect(autoscanMode.is(':disabled')).toBeFalsy()
      expect(autoscanLevel.is(':disabled')).toBeFalsy()
      expect(autoscanRecursive.is(':disabled')).toBeFalsy()
      expect(autoscanHidden.is(':disabled')).toBeFalsy()
      expect(autoscanInterval.is(':disabled')).toBeTruthy()
      item.scan_mode = previousMode
    })
  })

  describe('reset()', function () {
    it('resets all fields to empty', function () {
      var autoscanModal = $('#autoscanModal')
      autoscanModal.autoscanmodal('loadItem', {item: item})
      autoscanMode = $(':radio[name=autoscanMode]')
      autoscanLevel = $(':radio[name=autoscanLevel]')

      autoscanModal.autoscanmodal('reset')

      expect(autoscanId.val()).toBe('')
      expect(autoscanFromFs.is(':checked')).toBeFalsy()
      expect(autoscanMode.is(':checked')).toBeFalsy()
      expect(autoscanLevel.is(':checked')).toBeFalsy()
      expect(autoscanPersistent.is(':checked')).toBeFalsy()
      expect(autoscanRecursive.is(':checked')).toBeFalsy()
      expect(autoscanHidden.is(':checked')).toBeFalsy()
      expect(autoscanInterval.val()).toBe('')
      expect(autoscanPersistentMsg.css('display')).toBe('none')
      expect(autoscanSave.is(':disabled')).toBeFalsy()
    })
  })

  describe('saveItem()', function () {
    var autoscanModal

    beforeEach(function () {
      autoscanModal = $('#autoscanModal')
    })

    it('gives proper data for `TIMED` scan mode to save', function () {
      var previousMode = item.scan_mode
      item.scan_mode = 'timed'
      autoscanModal.autoscanmodal('loadItem', {item: item})

      var result = autoscanModal.autoscanmodal('saveItem')

      expect(result).toEqual({
        object_id: '2f6d',
        from_fs: true,
        scan_mode: 'timed',
        scan_level: 'full',
        recursive: true,
        hidden: true,
        interval: '1800'
      })
      item.scan_mode = previousMode
    })

    it('gives proper data for `INOTIFY` scan mode to save', function () {
      var previousMode = item.scan_mode
      item.scan_mode = 'inotify'
      autoscanModal.autoscanmodal('loadItem', {item: item})

      var result = autoscanModal.autoscanmodal('saveItem')

      expect(result).toEqual({
        object_id: '2f6d',
        from_fs: true,
        scan_mode: 'inotify',
        scan_level: 'full',
        recursive: true,
        hidden: true
      })

      item.scan_mode = previousMode
    })

    it('gives proper data for `NONE` scan mode to save', function () {
      var previousMode = item.scan_mode
      item.scan_mode = 'none'
      autoscanModal.autoscanmodal('loadItem', {item: item})

      var result = autoscanModal.autoscanmodal('saveItem')

      expect(result).toEqual({
        object_id: '2f6d',
        from_fs: true,
        scan_mode: 'none'
      })

      item.scan_mode = previousMode
    })
  })
})
