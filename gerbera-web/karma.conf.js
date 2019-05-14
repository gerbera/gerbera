module.exports = function(config) {
  config.set({
      files: [
        {
          pattern: 'test/client/fixtures/**/*',
        },
        {
          pattern: '../web/js/md5.js',
        },
        {
          pattern: 'test-context.js', watched: false
        },
        '../web/js/gerbera.app.js',
        '../web/js/gerbera.auth.js',
        '../web/js/gerbera.items.js',
        '../web/js/gerbera.tree.js',
        '../web/js/gerbera.menu.js',
        '../web/js/gerbera.trail.js',
        '../web/js/gerbera.autoscan.js',
        '../web/js/gerbera.updates.js',
        {
          pattern: 'spec-context.js', watched: false
        },
      ],
    frameworks : ['jasmine', 'fixture'],
    preprocessors: {
      'test-context.js': ['webpack', 'sourcemap'],
      'spec-context.js': ['webpack', 'sourcemap'],
      '**/*.html'   : ['html2js'],
      '**/*.json'   : ['json_fixtures']
    },
    webpack: {
      mode: 'development',
      devtool: 'inline-source-map',
      watch: true,
    },
    browsers: ['ChromeHeadless'],
    reporters: ['mocha'],
    concurrency: Infinity,
    singleRun: true
  });
};
