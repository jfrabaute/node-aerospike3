var assert = require('assert'),
    async = require('async'),
    aerospike = require('./aerospike');

var client = aerospike.createClient();

async.series([
  // Remove
  function(cb) {
     client.keyRemove({ns: "test", set: "set", key: "__TEST_KEY_OPS__"},
       function(err) {
         cb(null);
     });
  },
  // Put-ok
  function(cb) {
    client.keyPut({key: {ns: "test", set: "set", key: "__TEST_KEY_OPS__"},
      record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}},
      function(err) {
        assert.equal(err, undefined, "failed put-ok");
        cb(err);
    });
  },
  // Get-ok
  function(cb) {
    client.keyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OPS__"}},
      function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
        assert.deepEqual(result, {col1new: "value1-new", col2new: "value2-new", col3new: 3});
        cb(err);
    });
  },
  // Ops
  function(cb) {
    client.keyOperate({key:{ns: "test", set: "set", key: "__TEST_KEY_OPS__"},
                       ops: [
                          {op: "append_str", col: "col1new", value: "_post"},
                          {op: "incr", col: "col3new", value: 8},
                          {op: "prepend_str", col: "col2new", value: "pre_"},
                          {op: "read", col: "col1new"},
                          {op: "touch"},
                          {op: "write_str", col: "col4new", value: "test_write_ops"},
                          {op: "write_int64", col: "col5new", value: 124},
                       ]},
      function(err, result) {
        assert.equal(err, undefined, "failed ops-ok");
        assert.deepEqual(result, {col1new: "value1-new_post"}, "result ok");
        cb(err);
    });
  },
  // Get-ok
  function(cb) {
    client.keyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OPS__"}},
      function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
          assert.deepEqual(result, {col1new: "value1-new_post", col2new: "pre_value2-new",
                                    col3new: 11,
                                    col4new: "test_write_ops",
                                    col5new: 124});
        cb(err);
    });
  },
  // remove
  function(cb) {
     client.keyRemove({ns: "test", set: "set", key: "__TEST_KEY_OPS__"},
       function(err) {
         assert.equal(err, undefined, "failed remove");
         cb(err);
     });
  },
], function(err, results) {
  assert.equal(err, undefined);
})
