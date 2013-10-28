// node --expose-gc --trace-gc test-connect.js
// valgrind --leak-check=full --show-reachable=yes node --expose-gc --trace-gc test-connect.js

var assert = require('assert'),
    aerospike = require('../aerospike');

var client = aerospike.createClient({host: "1.1.1.1"});
client.on('error', function(err) {
  console.log("client.on('error') called");
  assert.equal(err.code, 200);
  assert.equal(client.isConnected(), false, "test client.isConnected()");
  assert.equal(client.isConnecting(), false, "test client.isConnecting()");

  client2 = aerospike.createClient();
  client2.on('error', function(err) {
    console.log("client2.on('error') called");
    assert.equal(err, undefined, "test connect ok");
    assert.equal(client.isConnected(), true, "test client.IsConnected() on connect");
    assert.equal(client.isConnecting(), false, "test client.IsConnecting() on connect");
    client.close(function(err) {
      assert.equal(err, undefined);
      assert.equal(client.isConnected(), false, "test client.IsConnected() on close");
      global.gc();
      console.log('END');
    });
  assert.equal(client2.isConnecting(), true);
  assert.equal(client2.isConnected(), false)
  });
  client2.on('connect', function() {
    console.log("client2.on('connect') called");
    assert.equal(client.isConnected(), true, "test client.IsConnected() on connect");
    assert.equal(client.isConnecting(), false, "test client.IsConnecting() on connect");
  });
});

assert.equal(client.isConnecting(), true);
assert.equal(client.isConnected(), false)
