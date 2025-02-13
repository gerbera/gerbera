import container from './fixtures/container';
import externalUrl from './fixtures/external-url';
import itemJson from './fixtures/item';

describe('The jQuery Gerbera Editor Overlay', () => {
  'use strict';
  let editObjectType;
  let editTitle;
  let editLocation;
  let editClass;
  let editDesc;
  let editMime;
  let editProtocol;
  let editSaveButton;
  let addParentId;
  let addParentIdTxt;
  let objectId;
  let objectIdTxt;
  let item;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
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

  afterEach(() => {
    fixture.cleanup();
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

    it('binds the onSave event to the Add button', () => {
      const saveSpy = jasmine.createSpy('save');
      const itemData = {type: 'external_url', item: item, onSave: saveSpy};

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
      editModal = $('#editModal');
    });

    it('should set fields in editor for `container`', () => {
      const itemData = {
        item: container
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('container');
      expect(editTitle.val()).toEqual('container title');
      expect(editLocation.val()).toEqual('');
      expect(editClass.val()).toEqual('object.container');
      expect(editDesc.val()).toEqual('');
      expect(editMime.val()).toEqual('');
      expect(editProtocol.val()).toEqual('');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('1471');
      expect(objectIdTxt.text()).toBe('1471');
    });

    it('should set fields in editor for `item`', () => {
      const itemData = {
        item: itemJson
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('item');
      expect(editTitle.val()).toEqual('Test.mp4');
      expect(editLocation.val()).toEqual('/folder/location/Test.mp4');
      expect(editClass.val()).toEqual('object.item.videoItem');
      expect(editDesc.val()).toEqual('A description');
      expect(editMime.val()).toEqual('video/mp4');
      expect(editProtocol.val()).toEqual('');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('39479');
      expect(objectIdTxt.text()).toBe('39479');
    });

    it('should set fields in editor for `external_url`', () => {
      const itemData = {
        item: externalUrl
      };

      editModal.editmodal('loadItem', itemData);

      expect(editObjectType.val()).toEqual('external_url');
      expect(editTitle.val()).toEqual('title');
      expect(editLocation.val()).toEqual('http://localhost');
      expect(editClass.val()).toEqual('object.item');
      expect(editDesc.val()).toEqual('description');
      expect(editMime.val()).toEqual('video/ts');
      expect(editProtocol.val()).toEqual('http-get');
      expect(editSaveButton.text()).toEqual('Save Item');
      expect(addParentId.val()).toBe('');
      expect(addParentIdTxt.text()).toBe('');
      expect(objectId.val()).toBe('1469');
      expect(objectIdTxt.text()).toBe('1469');
    });

    it('binds the onSave event to the save button', () => {
      const saveSpy = jasmine.createSpy('save');
      const itemData = {
        item: externalUrl,
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
      const itemData = { item: itemJson };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '39479',
        title: 'Test.mp4',
        description: 'A%20description',
        flags: 'Restricted',
        resources: '',
      });
    });

    it('gives proper data for `container` object to save', () => {
      const itemData = { item: container };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '1471',
        title: 'container%20title',
        flags: 'Restricted',
        resources: '',
      });
    });

    it('gives proper data for `external_url` object to save', () => {
      const itemData = { item: externalUrl };
      editModal.editmodal('loadItem', itemData);

      const result = editModal.editmodal('saveItem');

      expect(result).toEqual({
        object_id: '1469',
        title: 'title',
        location: 'http%3A%2F%2Flocalhost',
        description: 'description',
        'mime-type': 'video%2Fts',
        flags: 'Restricted',
        protocol: 'http-get',
        resources: '',
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
        flags: '',
        description: '',
        resources: '',
      });
    });

    it('gives proper data for `container` object to add', () => {
      editModal.editmodal('addNewItem', {type: 'container', item: item});

      const result = editModal.editmodal('addObject');

      expect(result).toEqual({
        parent_id: '9999',
        obj_type: 'container',
        class: 'object.container',
        flags: '',
        title: '',
        resources: '',
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
        'mime-type': '',
        flags: '',
        protocol: 'http-get',
        resources: '',
      });
    });
  });
});
