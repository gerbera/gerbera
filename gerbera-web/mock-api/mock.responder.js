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
        this.testCaseData = testCaseData;
        return require(`./${name}/tests/${testCaseData}`);
    }

};