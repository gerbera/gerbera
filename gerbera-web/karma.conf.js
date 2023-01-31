module.exports = function(config) {
  config.set({
      files: [
        {
          pattern: 'test/client/fixtures/**/*',
        },
        {
          pattern: '../web/vendor/md5.min.js',
        },
        {
          pattern: 'test-context.js', watched: false
        }
      ],
    frameworks : ['jasmine', 'fixture'],
    preprocessors: {
      'test-context.js': ['webpack', 'sourcemap'],
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
    singleRun: true,
    useIframe: false,
    processKillTimeout: 15000
  });
};
