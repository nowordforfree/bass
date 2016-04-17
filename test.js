'use strict';
const addon = require('./build/Release/bassplayer');

var player = new addon.BASSPlayer();
player.play('http://www.ex.ua/get/238573629');