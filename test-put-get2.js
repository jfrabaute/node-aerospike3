var assert = require('assert'),
    aerospike = require('./aerospike');

var client = aerospike.createClient();

var max = 1000;

client.Connect({}, function(err) {
    assert.equal(err, undefined, "failed connect");
    console.log('START PUT');
    var done = 0;
    for (i = 0 ; i < max ; i++)
    {
        client.KeyPut({key: {ns: "test", set: "set", key: "key_PUT" + i},
            record: {col1new: "value1-new" + i, col2new: "value2-new" + i, col3new: i}},
            function(err) {
                assert.equal(err, undefined, "error put " + i);
                done++;
                if (done == max) {
                    done = 0;
                    console.log('END PUT');
                    console.log('START GET');
                    for (var j = 0 ; j < max ; j++)
                    {
                        var k = j;
                        client.KeyGet({key: {ns: "test", set: "set", key: "key_PUT" + j}},
                            function(err, result) {
                                assert.equal(err, undefined, "error get " + k);
                                done++;
                                if (done == max) {
                                    console.log('END GET');
                                }
                        });
                    }
                }
        });
    }
})
