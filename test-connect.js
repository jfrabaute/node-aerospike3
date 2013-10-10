// node --expose-gc --trace-gc test-connect.js
// valgrind --leak-check=full --show-reachable=yes node --expose-gc --trace-gc test-connect.js

var assert = require('assert'),
    aerospike = require('./aerospike');

var client = aerospike.createClient();

var testConnectionOk = function() {
    console.log('testConnectionOk - start');
    client.Connect({host: "1.1.1.1"}, function(err) {
        assert.equal(err.code, 200);
        assert.equal(client.IsConnected(), false, "test client.IsConnected()");

        client.Connect({}, function(err) {
            assert.equal(err, undefined, "test connect ok");
            assert.equal(client.IsConnected(), true, "test client.IsConnected() on connect");
            client.Close(function(err) {
                assert.equal(err, undefined);
                assert.equal(client.IsConnected(), false, "test client.IsConnected() on close");
                global.gc();
                console.log('END');
                /*
                client.Connect({host: "1.1.1.1"}, function(err) {
                    assert.equal(err.code, 200);
                    assert.equal(client.IsConnected(), false, "test client.IsConnected()");
                });
                */
            });
        });
    });
}

var i = 0;
var testConnectionError = function() {
    i++;
    if (i < 2/*000*/)
        client.Connect({host: "127.0.0.1", port: 9876}, function(err) {
            assert.equal(err.code, 200);
            assert.equal(client.IsConnected(), false, "test client.IsConnected() - error");
            global.gc();
            testConnectionError();
        });
    else
        testConnectionOk();
}

testConnectionError();

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
