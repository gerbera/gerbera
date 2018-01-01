/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.toast.js - this file is part of Gerbera.

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
/* global $ */

$.widget('grb.toast', {

  _create: function () {
    var element = this.element
    element.addClass('grb-toast')
    this.span = element.find('span.grb-toast-msg')
    element.find('button.close').off('click').on('click', function () {
      element.hide()
    })
    element.hide()
  },

  _destroy: function () {
    this.element.children('span').text('')
    this.element.removeClass('grb-toast')
  },

  show: function (toast) {
    var element = this.element
    element.removeClass('grb-task')
    element.removeClass('alert-success')
    element.addClass('grb-toast')
    element.addClass('alert-warning')
    this.span.text(toast.message)
    var callback = toast.callback || function () { return false }
    if (!element.is(':visible')) {
      element.slideDown(callback)
    }
    if (this.close) {
      clearTimeout(this.close)
    }
    this.close = setTimeout(function () {
      element.slideUp()
    }, 5000)
  },

  showTask: function (toast) {
    var element = this.element
    element.removeClass('grb-toast')
    element.removeClass('alert-warning')
    element.addClass('grb-task')
    element.addClass('alert-success')

    this.span.text(toast.message)
    var callback = toast.callback || function () { return false }
    element.show(callback)
    if (this.close) {
      clearTimeout(this.close)
    }
    this.close = setTimeout(function () {
      element.hide()
    }, 5000)
  }
})
