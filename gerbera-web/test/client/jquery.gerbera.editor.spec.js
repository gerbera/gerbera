/* global jasmine it expect describe beforeEach loadJSONFixtures getJSONFixture loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures';
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures';

describe('The jQuery Gerbera Editor Overlay', () => {
  'use strict';
  let editObjectType;
  let editTitle;
  let editLocation;
  let editClass;
  let editDesc;
  let editMime;
  let editActionScript;
  let editState;
  let editProtocol;
  let editSaveButton;
  let addParentId;
  let addParentIdTxt;
  let objectId;
  let objectIdTxt;
  let item;

  beforeEach(() => {
    loadFixtures('edit-modal.html');
    editObjectType = $('#editObjectType');
    editTitle = $('#editTitle');
    editTitle.val('TEST');
    editLocation = $('#editLocation');
    editLocation.val('TEST');
    editClass = $('#editClass');
    editClass.val('TEST');
    editDesc = $('#editDesc');
    editDesc.val('TEST');
    editMime = $('#editMime');
    editMime.val('TEST');
    editActionScript = $('#editActionScript');
    editActionScript.val('TEST');
    editState = $('#editState');
    editState.val('TEST');
    editProtocol = $('#editProtocol');
    editProtocol.val('TEST');
    editSaveButton = $('#editSave');
    addParentId = $('#addParentId');
    addParentIdTxt = $('#addParentIdTxt');
    objectId = $('#objectId');
    objectIdTxt = $('#editdObjectIdTxt');
    item = {
      id: 12345
    };
  });

  describe('addNewItem()', () => {
    let editModal;

    beforeEach(() => {
      editModal = $('#editModal');
    });

    it('defaults fields based on type', () => {
      editModal.editmodal('addNewItem', {type: 'container', item: item});

      expect(editTitle.val()).toBe('');
      expect(editClass.val()).toBe('object.container');
    });

    it('makes object type enabled and not readonly', () => {
      editModal.editmodal('addNewItem', {type: 'container', item: item});

      expect(editObjectType.is(':disabled')).toBeFalsy();
      expect(editObjectType.prop('readonly')).toBe(false);
    });

    it('defaults editable fields when item selected', () => {
      editModal.editmodal('addNewItem', {type: 'item', item: item});

      expect(editTitle.val()).toBe('');
      expect(editLocation.val()).toBe('');
      expect(editLocation.is(':disabled')).toBeFalsy();
      expect(editClass.val()).toBe('object.item');
      expect(editDesc.val()).toBe('');
      expect(editMime.val()).toBe('');
    });

    it('defaults editable fields when active_item is selected', () => {
      editModal.editmodal('addNewItem', {type: 'active_item', item: item});

      expect(editTitle.val()).toBe('');
      expect(editLocation.val()).toBe('');
      expect(editLocation.is(':disabled')).toBeFalsy();
      expect(editClass.val()).toBe('object.item.activeItem');
      expect(editClass.is(':disabled')).toBeFalsy();
      expect(editDesc.val()).toBe('');
      expect(editMime.val()).toBe('');
      expect(editActionScript.val()).toBe('');
      expect(editState.val()).toBe('');
    });

    it('defaults editable fields when external_url is selected', () => {
      editModal.editmodal('addNewItem', {type: 'external_url', item: item});

      expect(editTitle.val()).toBe('');
      expect(editLocation.val()).toBe('');
      expect(editLocation.is(':disabled')).toBeFalsy();
      expect(editClass.val()).toBe('object.item');
      expect(editClass.is(':disabled')).toBeFalsy();
      expect(editDesc.val()).toBe('');
      expect(editMime.val()).toBe('');
      expect(editProtocol.val()).toBe('http-get');
    });

    it('defaults editable fields when internal_url is selected', () => {
      editModal.editmodal('addNewItem', {type: 'internal_url', item: item});

      expect(editTitle.val()).toBe('');
      expect(editLocation.val()).toBe('');
      expect(editLocation.is(':disabled')).toBeFalsy();
      expect(editClass.val()).toBe('object.item');
      expect(editClass.is(':disabled')).toBeFalsy();
      expect(editDesc.val()).toBe('');
      expect(editMime.val()).toBe('');
    });

    it('binds the onSave event to the Add button', () => {
      const saveSpy = jasmine.createSpy('save');
      const itemData = {type: 'internal_url', item: item, onSave: saveSpy};

      editModal.editmodal('addNewItem', itemData);
      $('#editSave').click();

      expect(saveSpy).toHaveBeenCalled();
    });
  });

  describe('reset()', () => {
    let editModal;

    beforeEach(() => {
      editModal = $('#editModal');
    });

    it('should reset fields to empty', () => {
      editModal.editmodal('reset');

      expect(editObjectType.val()).toEqual('item');
      expect(editTitle.val()).toEqual('');
      expect(editLocation.val()).toEqual('');
      expect(editClass.val()).toEqual('');
      expect(editDesc.val()).toEqual('');
      expect(editMime.val()).toEqual('');
      expect(editActionScript.val()).toEqual('');
      expect(editState.val()).toEqual('');
      expect(editProtocol.val()).toEqual('');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('');
      expect(objectIdTxt.text()).toBe('');
    });

    it('should hide parent and object ID fields', () => {
      editModal.editmodal('reset');

      expect(objectId.closest('.form-group').css('display')).toEqual('none');
      expect(objectIdTxt.closest('.form-group').css('display')).toEqual('none');
      expect(addParentId.closest('.form-group').css('display')).toEqual('none');
      expect(addParentIdTxt.closest('.form-group').css('display')).toEqual('none');
    });
  });

  describe('loadItem()', () => {
    let editModal;

    beforeEach(() => {
      loadJSONFixtures('active-item.json');
      loadJSONFixtures('container.json');
      loadJSONFixtures('external-url.json');
      loadJSONFixtures('internal-url.json');
      loadJSONFixtures('item.json');
      editModal = $('#editModal');
    });

    it('should set fields in editor for `container`', () => {
      const itemData = {
        item: getJSONFixture('container.json')
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('container');
      expect(editTitle.val()).toEqual('container title');
      expect(editLocation.val()).toEqual('');
      expect(editClass.val()).toEqual('object.container');
      expect(editDesc.val()).toEqual('');
      expect(editMime.val()).toEqual('');
      expect(editActionScript.val()).toEqual('');
      expect(editState.val()).toEqual('');
      expect(editProtocol.val()).toEqual('');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('1471');
      expect(objectIdTxt.text()).toBe('1471');
    });

    it('should set fields in editor for `item`', () => {
      const itemData = {
        item: getJSONFixture('item.json')
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('item');
      expect(editTitle.val()).toEqual('Test.mp4');
      expect(editLocation.val()).toEqual('/folder/location/Test.mp4');
      expect(editClass.val()).toEqual('object.item.videoItem');
      expect(editDesc.val()).toEqual('A description');
      expect(editMime.val()).toEqual('video/mp4');
      expect(editActionScript.val()).toEqual('');
      expect(editState.val()).toEqual('');
      expect(editProtocol.val()).toEqual('');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('39479');
      expect(objectIdTxt.text()).toBe('39479');
    });

    it('should set fields in editor for `active_item`', () => {
      const itemData = {
        item: getJSONFixture('active-item.json')
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('active_item');
      expect(editTitle.val()).toEqual('Test Active Item');
      expect(editLocation.val()).toEqual('/home/test.txt');
      expect(editClass.val()).toEqual('object.item.activeItem');
      expect(editDesc.val()).toEqual('test');
      expect(editMime.val()).toEqual('text/plain');
      expect(editActionScript.val()).toEqual('/home/echoText.sh');
      expect(editState.val()).toEqual('test-state');
      expect(editProtocol.val()).toEqual('');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('1306');
      expect(objectIdTxt.text()).toBe('1306');
    });

    it('should set fields in editor for `external_url`', () => {
      const itemData = {
        item: getJSONFixture('external-url.json')
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('external_url');
      expect(editTitle.val()).toEqual('title');
      expect(editLocation.val()).toEqual('http://localhost');
      expect(editClass.val()).toEqual('object.item');
      expect(editDesc.val()).toEqual('description');
      expect(editMime.val()).toEqual('text/plain');
      expect(editActionScript.val()).toEqual('');
      expect(editState.val()).toEqual('');
      expect(editProtocol.val()).toEqual('http-get');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('1469');
      expect(objectIdTxt.text()).toBe('1469');
    });

    it('should set fields in editor for `internal_url`', () => {
      const itemData = {
        item: getJSONFixture('internal-url.json')
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('internal_url');
      expect(editTitle.val()).toEqual('title');
      expect(editLocation.val()).toEqual('./test');
      expect(editClass.val()).toEqual('object.item');
      expect(editDesc.val()).toEqual('description');
      expect(editMime.val()).toEqual('text/plain');
      expect(editActionScript.val()).toEqual('');
      expect(editState.val()).toEqual('');
      expect(editProtocol.val()).toEqual('http-get');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('1470');
      expect(objectIdTxt.text()).toBe('1470');
    });

    it('binds the onSave event to the save button', () => {
      const saveSpy = jasmine.createSpy('save');
      const itemData = {
        item: getJSONFixture('internal-url.json'),
        onSave: saveSpy
      };

      editModal.editmodal('loadItem', itemData);
      $('#editSave').click();

      expect(saveSpy).toHaveBeenCalled();
    });
  });

  describe('saveItem()', () => {
    let editModal;

    beforeEach(() => {
      editModal = $('#editModal');
    });

    it('gives proper data for `item` object to save', () => {
      const itemData = { item: getJSONFixture('item.json') };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '39479',
        title: 'Test.mp4',
        description: 'A description'
      });
    });

    it('gives proper data for `container` object to save', () => {
      const itemData = { item: getJSONFixture('container.json') };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '1471',
        title: 'container title'
      });
    });

    it('gives proper data for `external_url` object to save', () => {
      const itemData = { item: getJSONFixture('external-url.json') };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '1469',
        title: 'title',
        location: 'http://localhost',
        description: 'description',
        protocol: 'http-get'
      });
    });

    it('gives proper data for `internal_url` object to save', () => {
      const itemData = { item: getJSONFixture('internal-url.json') };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '1470',
        title: 'title',
        location: './test',
        description: 'description',
        protocol: 'http-get'
      });
    });

    it('gives proper data for `active_item` object to save', () => {
      const itemData = { item: getJSONFixture('active-item.json') };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '1306',
        title: 'Test Active Item',
        location: '/home/test.txt',
        description: 'test',
        'mime-type': 'text/plain',
        action: '/home/echoText.sh',
        state: 'test-state'
      });
    });
  });

  describe('addObject()', () => {
    let editModal;
    let item;

    beforeEach(() => {
      editModal = $('#editModal');
      item = {
        id: 9999
      };
    });

    it('gives proper data for `item` object to add', () => {
      editModal.editmodal('addNewItem', {type: 'item', item: item});

      const result = editModal.editmodal('addObject');

      expect(result).toEqual({
        parent_id: '9999',
        obj_type: 'item',
        class: 'object.item',
        location: '',
        title: '',
        description: ''
      });
    });

    it('gives proper data for `container` object to add', () => {
      editModal.editmodal('addNewItem', {type: 'container', item: item});

      const result = editModal.editmodal('addObject');

      expect(result).toEqual({
        parent_id: '9999',
        obj_type: 'container',
        class: 'object.container',
        title: ''
      });
    });

    it('gives proper data for `external_url` object to add', () => {
      editModal.editmodal('addNewItem', {type: 'external_url', item: item});

      const result = editModal.editmodal('addObject');

      expect(result).toEqual({
        parent_id: '9999',
        obj_type: 'external_url',
        class: 'object.item',
        title: '',
        location: '',
        description: '',
        protocol: 'http-get'
      });
    });

    it('gives proper data for `internal_url` object to add', () => {
      editModal.editmodal('addNewItem', {type: 'internal_url', item: item});

      const result = editModal.editmodal('addObject');

      expect(result).toEqual({
        parent_id: '9999',
        obj_type: 'internal_url',
        class: 'object.item',
        title: '',
        location: '',
        description: '',
        protocol: ''
      });
    });

    it('gives proper data for `active_item` object to add', () => {
      editModal.editmodal('addNewItem', {type: 'active_item', item: item});

      const result = editModal.editmodal('addObject');

      expect(result).toEqual({
        parent_id: '9999',
        obj_type: 'active_item',
        class: 'object.item.activeItem',
        title: '',
        location: '',
        description: '',
        'mime-type': '',
        action: '',
        state: ''
      });
    });
  });
});
