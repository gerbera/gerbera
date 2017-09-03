module.exports = function (config) {
  config.set({
    basePath: '',
    frameworks: ['jasmine'],
    files: [
      // Vendor
      '../web/vendor/jquery/jquery-3.2.1.min.js',
      '../web/vendor/jquery/jquery.cookie.js',
      '../web/vendor/jquery/jquery-ui.min.js',
      '../web/vendor/bootstrap/js/bootstrap.js',
      // Unit Test
      'node_modules/jasmine-jquery/lib/jasmine-jquery.js',
      // Application
      '../web/js/md5.js',
      '../web/js/gerbera.app.js',
      '../web/js/gerbera.auth.js',
      '../web/js/gerbera.items.js',
      '../web/js/gerbera.tree.js',
      '../web/js/gerbera.menu.js',
      '../web/js/jquery.gerbera.items.js',
      '../web/js/jquery.gerbera.tree.js',
      // Tests
      'test/client/*.spec.js',
      {
        pattern: 'test/client/fixtures/**/*.json',
        watched: true,
        included: false,
        served: true
      },
      {
        pattern: 'test/client/fixtures/**/*.html',
        watched: true,
        included: false,
        served: true
      }
    ],
    exclude: [],
    preprocessors: {
      'js/*.js': 'coverage'
    },
    reporters: ['spec', 'coverage'],
    coverageReporter: {
      type: 'html',
      dir: 'coverage/'
    },
    port: 9876,
    colors: true,
    logLevel: config.LOG_INFO,
    autoWatch: false,
    singleRun: true,
    browsers: ['PhantomJS'],
    concurrency: Infinity
  })
}
