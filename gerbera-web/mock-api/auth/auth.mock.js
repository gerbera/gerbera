var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (action) {
    var count = ++requests[action].count
    var resp = requests[action].responses[count]
    if (resp === undefined) {
      resp = requests[action].responses['default']
    }
    return resp
  },
  reset: function (testName) {
    requests = require('./tests/' + testName)
  }
}
