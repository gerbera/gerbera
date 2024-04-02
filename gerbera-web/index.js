const path = require('path');
const express = require('express');
const app = express();
const staticPath = path.join(__dirname, '../web');
const imagePath = path.join(__dirname, 'mock-api');
const port = process.env.npm_package_config_webserver_port;

app.use(express.static(staticPath));
app.use(express.static(imagePath));

require('./mock-api/routes')(app);

app.listen(port, () => {
  console.log('Mock webserver listenting on : ' + port);
});
