const path = require('path');
const express = require('express');
const app = express();
const staticPath = path.join(__dirname, '../web');
const port = process.env.npm_package_config_webserver_port;

app.use(express.static(staticPath));

require('./mock-api/routes')(app);

app.listen(port, () => {
  console.log('Mock webserver listenting on : ' + port);
});
