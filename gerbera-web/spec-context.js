/*****************************
 * Gerbera Client Test Suite
 *****************************/
const tests = require.context('./test/client', true, /gerbera.*spec.js$/);
tests.keys().forEach(tests);