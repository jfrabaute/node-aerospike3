// node --expose-gc --trace-gc test-connect.js
// valgrind --leak-check=full --show-reachable=yes node --expose-gc --trace-gc test-connect.js

var assert = require('assert'),
    aerospike = require('./aerospike');

var client = aerospike.createClient({host: "1.1.1.1"});
client.on('error', function(err) {
  console.log("client.on('error') called");
  assert.equal(err.code, 200);
  assert.equal(client.isConnected(), false, "test client.isConnected()");
  assert.equal(client.isConnecting(), false, "test client.isConnecting()");
});

assert.equal(client.isConnecting(), true);
assert.equal(client.isConnected(), false)

client.keyExists({ns: "test", set: "set", key: "__TEST_KEY_OK__"}, function (err, result) {
  assert.equal(client.isConnecting(), false);
  assert.equal(client.isConnected(), false);
  assert.equal(err.code, -1);
});
