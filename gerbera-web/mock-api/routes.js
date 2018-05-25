const MockResponder = require('./mock.responder');
const addMock = new MockResponder('add');
const addObjectMock = new MockResponder('add_object');
const authMock = new MockResponder('auth');
const autoscanMock = new MockResponder('autoscan');
const containersMock = new MockResponder('containers');
const directoriesMock = new MockResponder('directories');
const editLoadMock = new MockResponder('edit_load');
const filesMock = new MockResponder('files');
const itemsMock = new MockResponder('items');
const removeMock = new MockResponder('remove');
const voidMock = new MockResponder('void');

module.exports = function (app) {
  app.get('/content/interface', (req, res) => {
    switch (req.query.req_type) {
      case 'add':
        res.send(require(addMock.getResponse(req.query.object_id)));
        break;
      case 'add_object':
        res.send(require(addObjectMock.getResponse(req.query.parent_id)));
        break;
      case 'autoscan':
        res.send(require(autoscanMock.getResponse(req.query.object_id, req.query.action)));
        break;
      case 'auth':
        res.send(require(authMock.getResponse(req.query.action)));
        break;
      case 'containers':
        res.send(require(containersMock.getResponse(req.query.parent_id, req.query.select_it)));
        break;
      case 'directories':
        res.send(require(directoriesMock.getResponse(req.query.parent_id, req.query.select_it)));
        break;
      case 'edit_load':
        res.send(require(editLoadMock.getResponse(req.query.object_id)));
        break;
      case 'files':
        res.send(require(filesMock.getResponse(req.query.parent_id, req.query.start, req.query.count)));
        break;
      case 'items':
        res.send(require(itemsMock.getResponse(req.query.parent_id, req.query.start)));
        break;
      case 'remove':
        res.send(require(removeMock.getResponse(req.query.object_id, req.query.all)));
        break;
      case 'void':
        res.send(require(voidMock.getResponse((req.query.updates || ''))));
        break;
    }
  });
  app.get('/reset', (req, res) => {
    const testName = req.query.testName;
    authMock.reset(testName);
    itemsMock.reset(testName);
    containersMock.reset(testName);
    res.sendStatus(200);
  });
};
