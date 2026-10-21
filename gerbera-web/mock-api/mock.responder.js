const fs = require('node:fs');
const path = require('node:path');

// Only source-controlled JSON fixtures discovered at startup can be selected.
// Request parameters are Map keys and never become filesystem/module paths.
const fixtures = new Map();
for (const component of fs.readdirSync(__dirname, { withFileTypes: true })) {
    if (!component.isDirectory()) continue;
    const directory = path.join(__dirname, component.name, 'tests');
    if (!fs.existsSync(directory) || !fs.lstatSync(directory).isDirectory()) continue;
    const cases = new Map();
    for (const fixture of fs.readdirSync(directory, { withFileTypes: true })) {
        if (fixture.isFile() && path.extname(fixture.name) === '.json') {
            cases.set(fixture.name, JSON.parse(fs.readFileSync(path.join(directory, fixture.name), 'utf8')));
        }
    }
    fixtures.set(component.name, cases);
}

module.exports = class MockResponder {

    constructor(mockName) {
        this.name = mockName;
        this.testCaseData = 'default.json';
        this.requests = this.getTestCase(mockName, 'default.json');
    }

    makeKey(args) {
        const result = [];
        Object.keys(args).forEach((key) => {
            result.push(args[key]);
        });
        return result.join('_');
    }

    getResponse() {
        const key = this.makeKey(arguments);

        try {
          const count = ++this.requests[key].count;
          let resp = this.requests[key].responses[count];
          if (resp === undefined) {
              resp = this.requests[key].responses['default'];
          }
          if (process.env.NODE_ENV === 'development') {
              console.log(`Get Response for [${this.name}] from ${this.testCaseData} with key = ${key} and count = ${count} -> ${resp}`); // eslint-disable-line
          }
          return resp;
        } catch {
            console.log(`Get Response for [${this.name}] from ${this.testCaseData} with key = ${key} failed`);
        }
        return "";
  }

    reset(testCaseData) {
        try {
            this.requests = this.getTestCase(this.name, testCaseData);
        } catch (error) {
            this.requests = this.getTestCase(this.name, 'default.json');
        }
    }

    getTestCase (name, testCaseData) {
        const cases = fixtures.get(name);
        if (!cases || !cases.has(testCaseData)) {
            throw new RangeError('Unknown mock fixture');
        }
        this.testCaseData = testCaseData;
        return structuredClone(cases.get(testCaseData));
    }

};
