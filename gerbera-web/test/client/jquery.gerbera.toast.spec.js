/* global jasmine it expect describe beforeEach loadFixtures */

jasmine.getFixtures().fixturesPath = 'base/test/client/fixtures'
jasmine.getJSONFixtures().fixturesPath = 'base/test/client/fixtures'

describe('The jQuery Gerbera Toast Message', function () {
  var toaster

  beforeEach(function () {
    loadFixtures('toast.html')
    toaster = $('#toast').toast()
  })

  describe('show()', function () {
    it('displays the toast message and invokes callback after animation', function (done) {
      var testExpects = function () {
        expect($('#grb-toast-msg').text()).toEqual('This is a message')
        expect($('#toast').css('display')).toEqual('block')
        done()
      }

      toaster.toast('show', {message: 'This is a message', callback: testExpects})
    })
  })

  describe('showTask()', function () {
    it('displays the task message', function (done) {
      var testExpects = function () {
        expect($('#grb-toast-msg').text()).toEqual('This is a message')
        expect($('#toast').css('display')).toEqual('block')
        done()
      }

      toaster.toast('showTask', {message: 'This is a message', callback: testExpects})
    })

    it('has specific task style', function (done) {
      var testExpects = function () {
        expect($('#toast').hasClass('grb-task')).toBeTruthy('Should have grb-task style')
        expect($('#toast').hasClass('alert-success')).toBeTruthy('Should have alert-success style')
        done()
      }

      toaster.toast('showTask', {message: 'This is a message', callback: testExpects})
    })
  })
})
