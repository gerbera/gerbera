var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (objectId, action) {
    // console.log('Add New Autoscan objectId=' + objectId + ' action=' + action)
    var count = ++requests[objectId + '_' + action].count
    var resp = requests[objectId + '_' + action].responses[count]
    if (resp === undefined) {
      resp = requests[objectId + '_' + action].responses['default']
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
