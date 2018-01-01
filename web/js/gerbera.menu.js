/*GRB*

    Gerbera - https://gerbera.io/

    gerbera.menu.js - this file is part of Gerbera.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2018 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/
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
     disable()
    }
    return $.Deferred().resolve().promise()
  }

  var click = function (event) {
    var menuItem = $(event.target)

    $('.nav-item').removeClass('active')
    menuItem.parent().addClass('active')

    var menuCommand = menuItem.data('gerbera-menu-cmd')
    switch (menuCommand) {
      case 'SELECT_TYPE':
        selectType(menuItem)
        break
      case 'LEAVE_BETA':
        leaveBeta()
        break
      case 'HOME':
        home()
        break
    }
  }

  var home = function () {
    GERBERA.Tree.destroy()
    GERBERA.Trail.destroy()
    GERBERA.Items.destroy()
  }

  var leaveBeta = function () {
    GERBERA.Tree.destroy()
    GERBERA.Trail.destroy()
    GERBERA.Items.destroy()
    GERBERA.App.reload('/index.html')
  }

  var selectType = function (menuItem) {
    var type = menuItem.data('gerbera-type')
    GERBERA.Tree.selectType(type, 0)
    GERBERA.App.setType(type)
    GERBERA.Items.destroy()
  }

  var disable = function () {
    var allLinks = $('nav li a')
    $('.nav li').removeClass('active')
    allLinks.addClass('disabled')
    allLinks.click(function () {
      return false
    })
    $('#report-issue').removeClass('disabled').off('click')
  }

  var hideLogin = function () {
    $('.login-field').hide()
    $('#login-submit').hide()
    $('#logout').hide()
  }

  var hideMenu = function () {
    $('ul.navbar-nav').hide()
  }

  return {
    initialize: initialize,
    disable: disable,
    hideLogin: hideLogin,
    hideMenu: hideMenu,
    click: click
  }
})()
