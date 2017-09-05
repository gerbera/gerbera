var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (parentId, selectIt) {
    var count = ++requests[parentId + '_' + selectIt].count
    var resp = requests[parentId + '_' + selectIt].responses[count]
    if (resp === undefined) {
      resp = requests[parentId + '_' + selectIt].responses['default']
    }
    return resp
  },
  reset: function (testName) {
    requests = require('./tests/' + testName)
  }
}
