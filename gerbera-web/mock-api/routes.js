var authMock = require('./auth/auth.mock')
var containersMock = require('./containers/containers.mock')
var directoriesMock = require('./directories/directories.mock')
var itemsMock = require('./items/items.mock')

module.exports = function (app) {
  app.get('/content/interface', function (req, res) {
    if (req.query.req_type === 'auth') {
      res.send(require(authMock.getResponse(req.query.action)))
    } else if (req.query.req_type === 'containers') {
      res.send(require(containersMock.getResponse(req.query.parent_id, req.query.select_it)))
    } else if (req.query.req_type === 'directories') {
      res.send(require(directoriesMock.getResponse(req.query.parent_id, req.query.select_it)))
    } else if (req.query.req_type === 'items') {
      res.send(require(itemsMock.getResponse(req.query.parent_id, req.query.start)))
    }
  })
  app.get('/reset', function (req, res) {
    authMock.reset(req.query.testName)
    // containersMock.reset(req.query.testName)
    // directoriesMock.reset(req.query.testName)
    res.sendStatus(200)
  })
}
