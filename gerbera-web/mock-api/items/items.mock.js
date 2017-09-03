var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (parentId, start) {
    var count = ++requests[parentId + '_' + start].count
    var resp = requests[parentId + '_' + start].responses[count]
    if (resp === undefined) {
      resp = requests[parentId + '_' + start].responses['default']
    }
    return resp
  },
  reset: function (testName) {
    requests = require('./tests/' + testName)
  }
}
