var Clay = require('pebble-clay');
var clayConfig = require('./config');
// Clay handles showConfiguration / webviewclosed and sends every messageKey to the watch.
var clay = new Clay(clayConfig);
