var assert = require('assert'),
    async = require('async'),
    aerospike = require('./aerospike');

var client = aerospike.createClient();

// Put-error
client.keyPut({key: {ns: "ns", set: "set", key: "key"},
  record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}},
  function(err, result) {
    assert.equal(err.code, 501, "error: Put-error");
});

// Key-error
client.keyExists({ns: "test", set: "set", key: "__THIS_KEY_DOES_NOT_EXISTS__"},
  function(err, result) {
    assert.equal(err, undefined, "error: Key-error");
    assert.equal(result, false);
});

async.series([
  // Put-ok
  function(cb) {
    client.keyPut({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
      record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}},
      function(err) {
        assert.equal(err, undefined, "failed put-ok");
        cb(err);
    });
  },
  // Exists-ok
  function(cb) {
    client.keyExists({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
      function(err, result) {
        assert.equal(err, undefined, "failed exists-ok");
        assert.equal(result, true);
        cb(err);
    });
  },
  // Get-ok
  function(cb) {
    client.keyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"}},
      function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
        assert.deepEqual(result, {col1new: "value1-new", col2new: "value2-new", col3new: 3});
        cb(err);
    });
  },
  // Get-ok
  function(cb) {
    client.keyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
      record: ["col1new", "col3new"]},
      function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
        assert.deepEqual(result, {col1new: "value1-new", col3new: 3});
        cb(err);
    });
  },
  // Remove-ok
  function(cb) {
    client.keyRemove({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
      function(err) {
        assert.equal(err, undefined, "failed remove-ok");
        cb(err);
    });
  },
  // Exists-ok-false
  function(cb) {
    client.keyExists({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
      function(err, result) {
        assert.equal(err, undefined, "failed exists-ok");
        assert.equal(result, false);
        cb(err);
    });
  }
], function(err, results) {
  assert.equal(err, undefined);
});
