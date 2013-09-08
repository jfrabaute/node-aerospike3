var assert = require('assert'),
    aerospike = require('./build/Release/aerospike');

var client1 = new aerospike.Client();
var client2 = new aerospike.Client();

client1.Connect({}, function(err) {
    assert.equal(err, undefined);
    assert.equal(client1.IsConnected(), true, "test client1.IsConnected() on connect");
    client1.Close(function(err) {
        assert.equal(err, undefined);
        assert.equal(client1.IsConnected(), false, "test client1.IsConnected() on close");
    });
});

client2.Connect({host: "1.1.1.1"}, function(err) {
    assert.equal(err, 200);
    assert.equal(client2.IsConnected(), false, "test client2.IsConnected()");
});
