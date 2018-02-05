#!/usr/bin/env node

var fs = require('fs');
var execSync = require('child_process').execSync;
var TOTAL_ITERATIONS = 50;

for(var i = 0; i < TOTAL_ITERATIONS; i++) {
  console.log('\nRunning Test Scenario # ' + i +'\n');
  execSync('npm run test', {stdio: 'inherit'});
  execSync('npm run test:e2e', {stdio: 'inherit'});
}

