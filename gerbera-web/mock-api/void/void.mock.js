var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (updates) {
    // console.log('Check Updates=' + updates)
    var count = ++requests[updates].count
    var resp = requests[updates].responses[count]
    if (resp === undefined) {
      resp = requests[updates].responses['default']
    }
    return resp
  },
  reset: function (testName) {
    requests = require('./tests/' + testName)
  }
}
