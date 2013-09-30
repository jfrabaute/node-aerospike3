var assert = require('assert'),
    async = require('async'),
    aerospike = require('./aerospike');

var client = new aerospike.Client();

client.Connect({}, function(err) {
  assert.equal(err, undefined, "failed connect");

  async.series([
    // Put-ok
    function(cb) {
      client.KeyPut({key: {ns: "test", set: "set", key: "__TEST_KEY_OPS__"},
        record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}},
        function(err) {
          assert.equal(err, undefined, "failed put-ok");
          cb(err);
      });
    },
    // Ops
    function(cb) {
      client.KeyOperate({key:{ns: "test", set: "set", key: "__TEST_KEY_OPS__"},
                         ops: [
                            {op: "append_str", col: "col1new", value: "_post"}
                         ]},
        function(err, result) {
          assert.equal(err, undefined, "failed ops-ok");
          assert.deepEqual(result, {}, "result ok");
          cb(err);
      });
    },
    // Get-ok
    function(cb) {
      client.KeyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OPS__"}},
        function(err, result) {
          assert.equal(err, undefined, "failed get-ok");
          assert.deepEqual(result, {col1new: "value1-new", col2new: "value2-new", col3new: 3});
          cb(err);
      });
    },
  ], function(err, results) {
    assert.equal(err, undefined);
  })
})
