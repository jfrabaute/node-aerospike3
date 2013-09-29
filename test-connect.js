var assert = require('assert'),
    aerospike = require('./aerospike');

var client = new aerospike.Client();

client.Connect({host: "1.1.1.1"}, function(err) {
    assert.equal(err.code, 200);
    assert.equal(client.IsConnected(), false, "test client.IsConnected()");

    client.Connect({}, function(err) {
        assert.equal(err, undefined, "test connect ok");
        assert.equal(client.IsConnected(), true, "test client.IsConnected() on connect");
        client.Close(function(err) {
            assert.equal(err, undefined);
            assert.equal(client.IsConnected(), false, "test client.IsConnected() on close");
            /*
            client.Connect({host: "1.1.1.1"}, function(err) {
                assert.equal(err.code, 200);
                assert.equal(client.IsConnected(), false, "test client.IsConnected()");
            });
            */
        });
    });
});

/*
for (var i = 0 ; i < 10 ; i++)
{
    c = new aerospike.Client();
    c.Connect({host: "192.0.2.0"}, function(err) {
        assert.equal(err.code, 200);
        console.log("c.IsConnected() = " + c.IsConnected());
        assert.equal(c.IsConnected(), false, "test c.IsConnected()");
    });
 }
*/
