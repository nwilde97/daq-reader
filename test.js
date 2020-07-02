
const addon = require('./build/Release/module');
let index = addon.setupDAQ();
console.log(`Setup DAQ`, index);
console.log("Scan", addon.scanChannels(index))
console.log(`Shutdown`, addon.shutdown());