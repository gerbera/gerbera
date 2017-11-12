var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (parentId) {
    // console.log('Add Object parentId=' + objectId)
    var count = ++requests[parentId].count
    var resp = requests[parentId].responses[count]
    if (resp === undefined) {
      resp = requests[parentId].responses['default']
    }
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
