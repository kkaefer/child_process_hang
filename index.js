var blend = require('./build/default/blend').blend;
var Buffer = require('buffer').Buffer;
var exec = require('child_process').exec;

var image = require('fs').readFileSync('image.png');


var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A', 'binary');
blend([ buffer, image ], function(err, data) {
    // This produces an error.
    console.warn('blend returned');

    exec('echo foo', function(err, stdout, stderr) {
        console.warn('exec returned');
    });
});
