var assert = require('assert'),
    async = require('async'),
    aerospike = require('./aerospike');

var client = aerospike.createClient();

client.Connect({}, function(err) {
  assert.equal(err, undefined, "failed connect");

  async.parallel([
    // Put-error
    function(cb) {
      client.KeyPut({key: {ns: "ns", set: "set", key: "key"},
        record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}},
        function(err, result) {
          assert.equal(err.code, 501, "error: Put-error");
          cb((err && err.code == 501) ? undefined : err);
      });
    },
    // Key-error
    function(cb) {
      client.KeyExists({ns: "test", set: "set", key: "__THIS_KEY_DOES_NOT_EXISTS__"},
        function(err, result) {
          assert.equal(err, undefined, "error: Key-error");
          assert.equal(result, false);
          cb(err);
      });
    },
  ], function(err, results) {
    assert.equal(err, undefined);
  })

  async.series([
    // Put-ok
    function(cb) {
      client.KeyPut({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
        record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}},
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
    // Get-ok
    function(cb) {
      client.KeyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"}},
        function(err, result) {
          assert.equal(err, undefined, "failed get-ok");
          assert.deepEqual(result, {col1new: "value1-new", col2new: "value2-new", col3new: 3});
          cb(err);
      });
    },
    // Get-ok
    function(cb) {
      client.KeyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
        record: ["col1new", "col3new"]},
        function(err, result) {
          assert.equal(err, undefined, "failed get-ok");
          assert.deepEqual(result, {col1new: "value1-new", col3new: 3});
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
