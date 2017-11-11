var requests = require('./tests/default.json')

module.exports = {
  getResponse: function (parentId, start, pageCount) {
    // console.log('Load Files Parent ID=' + parentId + ' start=' + start + ' count=' + count)
    var count = ++requests[parentId + '_' + start + '_' + pageCount].count
    var resp = requests[parentId + '_' + start + '_' + pageCount].responses[count]
    if (resp === undefined) {
      resp = requests[parentId + '_' + start + '_' + pageCount].responses['default']
    }
    return resp
  },
  reset: function (testName) {
    requests = require('./tests/' + testName)
  }
}
