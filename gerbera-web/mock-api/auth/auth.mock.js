var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (action) {
    var count = ++requests[action].count
    var resp = requests[action].responses[count]
    if (resp === undefined) {
      resp = requests[action].responses['default']
    }
    //console.log('Auth Mock serving for ' + count + ' [' + action + '] --> ' + resp)
    return resp
  },
  reset: function (testName) {
    try {
      requests = require('./tests/' + testName)
    } catch (error) {
      requests = require('./tests/default.json')
    }
  }
}
