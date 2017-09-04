var GERBERA
if (typeof (GERBERA) === 'undefined') {
  GERBERA = {}
}

GERBERA.Menu = (function () {
  'use strict'

  var initialize = function () {
    var allLinks = $('nav li a')
    if (GERBERA.Auth.isLoggedIn()) {
      allLinks.click(GERBERA.Menu.click)
      allLinks.removeClass('disabled')
      $('#nav-home').click()
    } else {
      $('.nav li').removeClass('active')
      allLinks.addClass('disabled')
      allLinks.click(function () {
        return false
      })
      $('#report-issue').removeClass('disabled').off('click')
    }
    return $.Deferred().resolve().promise()
  }

  var click = function (event) {
    var menuItem = $(event.target)

    $('.nav li').removeClass('active')
    menuItem.parent().addClass('active')

    var menuCommand = menuItem.data('gerbera-menu-cmd')
    switch (menuCommand) {
      case 'SELECT_TYPE':
        selectType(menuItem)
        break
      case 'HOME':
        home()
        break
    }
  }

  var home = function () {
    GERBERA.Tree.destroy()
    GERBERA.Items.destroy()
    $('#item-breadcrumb').html('<li>Select a type</li>')
  }

  var selectType = function (menuItem) {
    var type = menuItem.data('gerbera-type')
    GERBERA.Tree.selectType(type, 0)
    GERBERA.App.setType(type)
    GERBERA.Items.destroy()
  }

  return {
    initialize: initialize,
    click: click
  }
})()
