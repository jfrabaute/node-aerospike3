var aerospike = require('./build/Release/aerospike');

console.log('aerospike: ' + aerospike);

console.log('aerospike.Client: ' + aerospike.Client);
var client = new aerospike.Client();
console.log('new aerospike.Client: ' + client);
console.log('client.Connect: ' + client.Connect);
console.log('client.Close: ' + client.Close);
ok = false;
try {
  client.Connect()
}
catch (err) {
  ok = (err == "TypeError: Missing config argument (object or array of objects)");
}
if (!ok)
  throw "error";

ok = false;
try {
  client.Connect(1);
}
catch (err) {
  ok = (err == "TypeError: Wrong argument type (must be either object or array of objects)");
}
if (!ok)
  throw "error";

ok = false;
try {
  client.Connect({});
}
catch (err) {
  ok = (err == "TypeError: Missing or non string \"addr\" property in config object");
}
if (!ok)
 throw "error";

ok = false;
try {
  client.Connect({addr: 1});
}
catch (err) {
  ok = (err == "TypeError: Missing or non string \"addr\" property in config object");
}
if (!ok)
  throw "error";

ok = false;
try {
  client.Connect({addr: "localhost", port: "a"});
}
catch (err) {
  ok = (err == "TypeError: Missing or non unsigned int \"port\" property in config object");
}
if (!ok)
  throw "error";

console.log("client.IsConnected(): " + client.IsConnected());
client.Connect({addr: "1.1.1.1", port: 8000}, function(err) {
  if (!err)
    throw "error";
});

if (client.IsConnected())
  throw "error";

client.Connect({addr: "localhost", port: 3000}, function(err) {
  if (err)
    throw "error";
});


if (!client.IsConnected())
  throw "error";

console.log('client.Close(): ' + client.Close());

if (client.IsConnected())
  throw "error";

console.log('END TEST');
