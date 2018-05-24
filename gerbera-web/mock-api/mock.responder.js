module.exports = class MockResponder {

    constructor(mockName) {
        this.name = mockName;
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
        const count = ++this.requests[key].count;
        if (process.env.NODE_ENV === 'development') {
            console.log(`Get Response for [${this.name}] with key = ${key} and count = ${count}`); // eslint-disable-line
        }
        let resp = this.requests[key].responses[count];
        if (resp === undefined) {
            resp = this.requests[key].responses['default'];
        }
        return resp;
    }

    reset(testCaseData) {
        try {
            this.requests = this.getTestCase(this.name, testCaseData);
        } catch (error) {
            this.requests = this.getTestCase(this.name, 'default.json');
        }
    }

    getTestCase (name, testCaseData) {
        return require(`./${name}/tests/${testCaseData}`);
    }

};