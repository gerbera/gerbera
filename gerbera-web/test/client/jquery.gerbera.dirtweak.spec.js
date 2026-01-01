/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.dirtweak.spec.js - this file is part of Gerbera.

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
import dirTweakItem from './fixtures/dirtweak-item';
import emptyTweakItem from './fixtures/dirtweak-item-empty';
import newTweakItem from './fixtures/dirtweak-item-new';

describe('The jQuery Gerbera DirTweak Overlay', () => {
  'use strict';
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

  describe('loadItem()', () => {
    it('loads the dirtweak item into the fields', () => {
      dirTweakModal.dirtweakmodal('loadItem', dirTweakItem);

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
    });
    it('loads empty dirtweak item into the fields', () => {
      dirTweakModal.dirtweakmodal('loadItem', emptyTweakItem);

      expect(dirTweakLocation.val()).toBe('/home/gerbera/Music');
      expect(dirTweakInherit.is(':checked')).toBeFalsy();
      expect(dirTweakSymLinks.is(':checked')).toBeFalsy();
      expect(dirTweakRecursive.is(':checked')).toBeFalsy();
      expect(dirTweakCaseSens.is(':checked')).toBeFalsy();
      expect(dirTweakMetaCharset.val()).toBe('');
      expect(dirTweakFanArt.val()).toBe('');
      expect(dirTweakSubtitle.val()).toBe('');
      expect(dirTweakResource.val()).toBe('');
      expect(dirTweakDelete.css('display')).toBe('none');
      expect(dirTweakSave.is(':disabled')).toBeFalsy();
    });
  });

  describe('saveItem()', () => {
    it('gives proper data on save', () => {
      dirTweakModal.dirtweakmodal('loadItem', emptyTweakItem);

      dirTweakInherit.prop('checked', true);
      const result = dirTweakModal.dirtweakmodal('saveItem');

      expect(result).toEqual(newTweakItem);
    });
  });
});
