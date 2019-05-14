describe('The jQuery Gerbera Toast Message', () => {
  let toaster;

  beforeEach(() => {
    fixture.setBase('test/client/fixtures');
    fixture.load('index.html');
    toaster = $('#toast').toast();
  });

  afterEach(() => {
    fixture.cleanup();
  });

  describe('show()', () => {
    it('displays the toast message and invokes callback after animation', (done) => {
      const testExpects = () => {
        expect($('#grb-toast-msg').text()).toEqual('This is a message');
        expect($('#toast').css('display')).toEqual('block');
        done();
      };

      toaster.toast('show', {message: 'This is a message', callback: testExpects});
    });
  });

  describe('showTask()', () => {
    it('displays the task message', (done) => {
      const testExpects = () => {
        expect($('#grb-toast-msg').text()).toEqual('This is a message');
        expect($('#toast').css('display')).toEqual('block');
        done();
      };

      toaster.toast('showTask', {
        message: 'This is a message',
        callback: testExpects,
        type: 'success',
        icon: 'fa-check'
      });
    });

    it('has specific task style', (done) => {
      const testExpects = () => {
        const toaster = $('#toast');
        expect(toaster.hasClass('grb-task')).toBeTruthy('Should have grb-task style');
        expect(toaster.hasClass('alert-success')).toBeTruthy('Should have alert-success style');
        done();
      };

      toaster.toast('showTask', {
        message: 'This is a message',
        callback: testExpects,
        type: 'success',
        icon: 'fa-check'
      });
    });
  });
});
