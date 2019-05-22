/*GRB*

    Gerbera - https://gerbera.io/

    gerbera-menu.module.js - this file is part of Gerbera.

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

import {Items} from "./gerbera-items.module.js";
import {GerberaApp} from "./gerbera-app.module.js";
import {Trail} from "./gerbera-trail.module.js";
import {Tree} from "./gerbera-tree.module.js";

const disable = () => {
  const allLinks = $('nav li a');
  $('.nav li').removeClass('active');
  allLinks.addClass('disabled');
  allLinks.click(function () {
    return false
  });
  $('#report-issue').removeClass('disabled').off('click');
};

const hideLogin = () => {
  $('.login-field').hide();
  $('#login-submit').hide();
  $('#logout').hide();
};

const hideMenu = () => {
  $('ul.navbar-nav').hide();
};

const initialize = () => {
  const allLinks = $('nav li a');
  if (GerberaApp.isLoggedIn()) {
    allLinks.click(Menu.click);
    allLinks.removeClass('disabled');
    $('#nav-home').click();
    if(GerberaApp.serverConfig.friendlyName) {
      $('#nav-home').text('Home [' + GerberaApp.serverConfig.friendlyName +']');
    }
    const version = $('#gerbera-version');
    if(GerberaApp.serverConfig.version) {
      version.children('span').text(GerberaApp.serverConfig.version);
      version.parent('li').removeAttr('hidden');
    } else {
      version.parent('li').attr('hidden');
    }
  } else {
    disable();
  }
  return Promise.resolve();
};

const selectType = (menuItem) => {
  $('#home').hide();
  $('#content').show();
  const type = menuItem.data('gerbera-type');
  Tree.selectType(type, 0);
  GerberaApp.setType(type);
  Items.destroy();
};

var click = (event) => {
  const menuItem = $(event.target).closest('.nav-link');

  if (!menuItem.hasClass("noactive")) {
    $('.nav-item').removeClass('active');
    menuItem.closest('li').addClass('active');
  }

  const menuCommand = menuItem.data('gerbera-menu-cmd');
  switch (menuCommand) {
    case 'SELECT_TYPE':
      selectType(menuItem);
      break;
    case 'HOME':
      home();
      break
  }
};

var home = function () {
  $('#home').show();
  $('#content').hide();
  Tree.destroy();
  Trail.destroy();
  Items.destroy();
};


export const Menu = {
  click,
  home,
  disable,
  hideLogin,
  hideMenu,
  initialize,
  selectType,
};