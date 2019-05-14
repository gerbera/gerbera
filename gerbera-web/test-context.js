/*****************************
 * Vendor Support
 *****************************/
require("@babel/polyfill");

const Cookies = require("js-cookie");
window.Cookies = Cookies;

const $ = require('jquery');
window.jQuery = $;
window.$ = $;

require('jquery-ui');
require('popper.js');
require('bootstrap');

/*****************************
 * Gerbera JQuery Plugins
 *****************************/
const plugins = require.context('../web/js', true, /jquery.gerbera.*.js$/);
plugins.keys().forEach(plugins);