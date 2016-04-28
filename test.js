var addon = require('./main');

var myObject = new addon();

// emit event sample
myObject.on("ping", s => console.log("Event ping. Received " + s));
myObject.on("someEvent", a => console.log("Event emitted from C Land with argument " + a));

myObject.call_emit();

// callback sample
myObject.callbackSample(1500, () => console.log("yeah, callback invoked after async call in C land"));

myObject.startLoop();

setInterval(() => {}, 5000);