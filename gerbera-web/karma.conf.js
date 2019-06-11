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
    singleRun: true
  });
};
