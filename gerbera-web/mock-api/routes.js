var authMock = require('./auth/auth.mock')
var containersMock = require('./containers/containers.mock')
var directoriesMock = require('./directories/directories.mock')
var editLoadMock = require('./edit_load/edit.load.mock')
var filesMock = require('./files/files.mock')
var itemsMock = require('./items/items.mock')
var removeMock = require('./remove/remove.mock')
var addMock = require('./add/add.mock')
var addObjectMock = require('./add_object/add_object.mock')
var autoscanMock = require('./autoscan/autoscan.mock')
var voidMock = require('./void/void.mock')

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
    } else if (req.query.req_type === 'remove') {
      res.send(require(removeMock.getResponse(req.query.object_id, req.query.all)))
    } else if (req.query.req_type === 'edit_load') {
      res.send(require(editLoadMock.getResponse(req.query.object_id)))
    } else if (req.query.req_type === 'files') {
      res.send(require(filesMock.getResponse(req.query.parent_id, req.query.start, req.query.count)))
    } else if (req.query.req_type === 'add') {
      res.send(require(addMock.getResponse(req.query.object_id)))
    } else if (req.query.req_type === 'add_object') {
      res.send(require(addObjectMock.getResponse(req.query.parent_id)))
    } else if (req.query.req_type === 'autoscan') {
      res.send(require(autoscanMock.getResponse(req.query.object_id, req.query.action)))
    } else if (req.query.req_type === 'void') {
      res.send(require(voidMock.getResponse((req.query.updates || ''))))
    }
  })
  app.get('/reset', function (req, res) {
    authMock.reset(req.query.testName)
    itemsMock.reset(req.query.testName)
    containersMock.reset(req.query.testName)
    res.sendStatus(200)
  })
}
