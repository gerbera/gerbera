/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.trail.spec.js - this file is part of Gerbera.

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

/* global fixture */
import trailDataJson from './fixtures/trail-data';

describe('The jQuery Trail', () => {
  'use strict';
  let trailData, trail;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('trail.html');
    trail = $('#trail');
    trailData = trailDataJson;
  });

  afterEach(() => {
    if (trail.hasClass('grb-trail')) {
      trail.trail('destroy')
    }
    fixture.cleanup();
  });

  it('Uses an array of items to build the breadcrumb', () => {
    const trailConfig = {};

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    expect(trail.trail('length')).toBe(4)
  });

  it('should display in the proper order', () => {
    const trailConfig = {};
    trailData = [
      {
        text: 'First Item',
        gerbera: {
          id: 0
        }
      },
      {
        text: 'Second Item',
        gerbera: {
          id: 0
        }
      }
    ];

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    const items = trail.trail('items');
    expect(items.first().text()).toBe('First Item');
    expect(items.last().text()).toBe('Second Item');
  });

  it('generates add icons when configured', () => {
    const trailConfig = {
      enableAdd: true,
      onAdd: () => { return false },
      enableAddAutoscan: true,
      onAddAutoscan: () => { return false }
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    expect($('#trail .grb-trail-add').length).toBe(1);
    expect($('#trail .grb-trail-add-autoscan').length).toBe(1);
  });

  it('does not generate add icons when not enabled by configuration', () => {
    const trailConfig = {
      enableAdd: false,
      enableAddAutoscan: false
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    expect($('#trail .grb-trail-add').length).toBe(0);
    expect($('#trail .grb-trail-add-autoscan').length).toBe(0);
  });

  it('generates delete icon when enabled by configuration', () => {
    const trailConfig = {
      enableDelete: true,
      onDelete: () => { return false },
      enableDeleteAll: true,
      onDeleteAll: () => { return false },
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    expect($('#trail .grb-trail-delete').length).toBe(1);
    expect($('#trail .grb-trail-delete-all').length).toBe(1);
  });

  it('binds the delete icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('delete');
    const trailConfig = {
      enableDelete: true,
      onDelete: methodSpy
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    trail.find('a.grb-trail-delete').click();

    expect(methodSpy).toHaveBeenCalled();
  });

  it('binds the delete all icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('deleteAll');
    const trailConfig = {
      enableDeleteAll: true,
      onDeleteAll: methodSpy
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    trail.find('a.grb-trail-delete-all').click();

    expect(methodSpy).toHaveBeenCalled();
  });

  it('does not generate delete icons when not configured', () => {
    const trailConfig = {
      enableDelete: false,
      enableDeleteAll: false
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    expect($('#trail .grb-trail-delete').length).toBe(0);
    expect($('#trail .grb-trail-delete-all').length).toBe(0);
  });

  it('binds the add icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('add');
    const trailConfig = {
      enableAdd: true,
      onAdd: methodSpy
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    trail.find('a.grb-trail-add').click();

    expect(methodSpy).toHaveBeenCalled();
  });

  it('binds the add autoscan icon click to the method provided', () => {
    const methodSpy = jasmine.createSpy('addAutoscan');
    const trailConfig = {
      enableAddAutoscan: true,
      onAddAutoscan: methodSpy
    };

    trail.trail({
      data: trailData,
      config: trailConfig
    });

    trail.find('a.grb-trail-add-autoscan').click();

    expect(methodSpy).toHaveBeenCalled();
  });

  describe('length()', () => {
    it('reports the length of the trail of items', () => {
      const trailConfig = {};

      trail.trail({
        data: trailData,
        config: trailConfig
      });
      expect(trail.trail('length')).toBe(4);
    })
  });

  describe('items()', () => {
    it('returns the list of items', () => {
      const trailConfig = {};

      trail.trail({
        data: trailData,
        config: trailConfig
      });

      const items = trail.trail('items');
      expect(items.length).toBe(4);
      expect(items.first().text()).toBe('database');
    });
  });

  describe('destroy()', () => {
    it('removes all contents of the parent', () => {
      const trailConfig = {};

      trail.trail({
        data: trailData,
        config: trailConfig
      });
      expect(trail.trail('length')).toBe(4);

      trail.trail('destroy');

      expect(trail.children().length).toBe(0);
      expect(trail.hasClass('grb-trail')).toBeFalsy();
    });
  });
});
