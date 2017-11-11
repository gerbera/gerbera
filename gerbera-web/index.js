var path = require('path')
var express = require('express')
var app = express()
var staticPath = path.join(__dirname, '../web')
var port = process.env.npm_package_config_webserver_port

app.use(express.static(staticPath))

require('./mock-api/routes')(app)

app.listen(port, function () {
  console.log('Mock webserver listenting on : ' + port)
})
