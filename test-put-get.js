var assert = require('assert'),
    async = require('async');

try {
  var aerospike = require('./build/Debug/aerospike');
} catch (error) {
  var aerospike = require('./build/Release/aerospike');
}
    

var client = new aerospike.Client();


client.Connect({}, function(err) {
  assert.equal(err, undefined, "failed connect");

  async.parallel([
    // Put-error
    function(cb) {
      client.KeyPut({ns: "ns", set: "set", key: "key"}, 
        {col1new: "value1-new", col2new: "value2-new", col3new: 3},
        function(err, result) {
          assert.equal(err, 501, "error: Put-error");
          cb(err == 501 ? undefined : err);
      });
    },
    // Key-error
    function(cb) {
      client.KeyExists({ns: "test", set: "set", key: "__THIS_KEY_DOES_NOT_EXISTS__"},
        function(err, result) {
          assert.equal(err, undefined, "error: Key-error");
          assert.equal(result, false);
          cb(err == 501 ? undefined : err);
      });
    },
  ], function(err, results) {
    assert.equal(err, undefined);
  })

  async.series([
    // Put-ok
    function(cb) {
      client.KeyPut({ns: "test", set: "set", key: "__TEST_KEY_OK__"}, 
        {col1new: "value1-new", col2new: "value2-new", col3new: 3},
        function(err) {
          assert.equal(err, undefined, "failed put-ok");
          cb(err);
      });
    },
    // Exists-ok
    function(cb) {
      client.KeyExists({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
        function(err, result) {
          assert.equal(err, undefined, "failed exists-ok");
          assert.equal(result, true);
          cb(err);
      });
    },
    // Remove-ok
    function(cb) {
      client.KeyRemove({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
        function(err) {
          assert.equal(err, undefined, "failed remove-ok");
          cb(err);
      });
    },
    // Exists-ok-false
    function(cb) {
      client.KeyExists({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
        function(err, result) {
          assert.equal(err, undefined, "failed exists-ok");
          assert.equal(result, false);
          cb(err);
      });
    },
  ], function(err, results) {
    assert.equal(err, undefined);
  })
})
