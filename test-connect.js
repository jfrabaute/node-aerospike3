var aerospike = require('./build/Release/aerospike');

var client = new aerospike.Client();

client.Connect({}, function(err) {
  console.log('in client.Connect callback.');
  console.log(err);
  console.log("client.isconnected = " + client.IsConnected());
});
console.log('after client.Connect');
