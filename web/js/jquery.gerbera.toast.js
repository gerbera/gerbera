/*GRB*

    Gerbera - https://gerbera.io/

    jquery.gerbera.toast.js - this file is part of Gerbera.

    Copyright (C) 2016-2019 Gerbera Contributors

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
    this.span = element.find('span#grb-toast-msg')
    this.icon = element.find('i#grb-toast-icon')
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
    element.removeClass()
    element.addClass('grb-toast alert alert-' + toast.type)
    this.span.text(toast.message)
    this.icon.removeClass()
    this.icon.addClass("fa " + toast.icon)
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
    element.removeClass()
    element.addClass('grb-task alert alert-' + toast.type)
    this.span.text(toast.message)
    this.icon.removeClass()
    this.icon.addClass("fa " + toast.icon)
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
