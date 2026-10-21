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

test('fixture names cannot select files outside the fixture catalogue', () => {
  const mock = new MockResponder('auth');
  for (const name of ['../config.json', '../../../package.json', '../../mock.responder.js',
    '/etc/passwd', 'C:\\Windows\\win.ini', '..\\config.json', '__proto__', 'default.json\0']) {
    assert.throws(() => mock.getTestCase('auth', name), RangeError, name);
  }
});

test('component names must exist in the fixture catalogue', () => {
  for (const name of ['../auth', 'auth/..', '/tmp', '__proto__', 'constructor']) {
    assert.throws(() => new MockResponder(name), RangeError, name);
  }
});

test('an unknown reset fixture keeps the established default response', () => {
  const mock = new MockResponder('auth');
  mock.reset('../../../package.json');
  assert.equal(mock.testCaseData, 'default.json');
  assert.equal(mock.getResponse('get_config'), './auth/config.json');
});

test('fixture response contents are independent as well as counters', () => {
  const mock = new MockResponder('auth');
  const first = mock.getTestCase('auth', 'login.test.requests.json');
  first.get_config.responses.default = 'changed';
  const second = mock.getTestCase('auth', 'login.test.requests.json');
  assert.notEqual(second.get_config.responses.default, 'changed');
});
