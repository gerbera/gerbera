var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (objectId, all) {
    var count = ++requests[objectId + '_' + all].count
    var resp = requests[objectId + '_' + all].responses[count]
    if (resp === undefined) {
      resp = requests[objectId + '_' + all].responses['default']
    }
    return resp
  },
  reset: function (testName) {
    requests = require('./tests/' + testName)
  }
}
