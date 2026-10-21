/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera. If not, see <http://www.gnu.org/licenses/>.
*/
const { test } = require('node:test');
const assert = require('node:assert/strict');
const MockResponder = require('../../mock-api/mock.responder');

test('reset restarts a response sequence even when the fixture was cached', () => {
  const mock = new MockResponder('auth');
  mock.reset('login.test.requests.json');
  assert.equal(mock.getResponse('get_config'), './auth/no-accounts-config.json');
  assert.equal(mock.getResponse('get_config'), './auth/config.json');
  mock.reset('login.test.requests.json');
  assert.equal(mock.getResponse('get_config'), './auth/no-accounts-config.json');
});

test('responders do not share response counters', () => {
  const first = new MockResponder('auth');
  const second = new MockResponder('auth');
  first.reset('login.test.requests.json');
  second.reset('login.test.requests.json');
  assert.equal(first.getResponse('get_sid'), './auth/get_sid.logged_in.json');
  assert.equal(second.getResponse('get_sid'), './auth/get_sid.logged_in.json');
  assert.equal(first.getResponse('get_sid'), './auth/get_sid.not.logged_in.json');
});
