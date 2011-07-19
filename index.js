var jump = require('./build/default/jump').jump;
var exec = require('child_process').exec;

jump(function(err, data) {
    console.warn('jump returned');

    exec('echo foo', function(err, stdout, stderr) {
        console.warn('exec returned');
    });
});
