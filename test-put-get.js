var assert = require('assert'),
    async = require('async');

try {
  var aerospike = require('./build/Debug/aerospike');
} catch (error) {
  var aerospike = require('./build/Release/aerospike');
}
    

var client = new aerospike.Client();

async.series([
  function(cb) {
    client.Connect({}, function(err) {
      assert.equal(err, undefined, "failed connect");
      cb(err);
    })
  },
  function(cb) {
    client.KeyPut({ns: "test", set: "set", key: "key"}, 
      {col1new: "value1-new", col2new: "value2-new", col3new: 3},
      function(err, result) {
        assert.equal(err, undefined, "failed put");
        cb(err);
    });
  }
]);

/*
console.log("client.KeyExists");
client.KeyExists({ns: "ns", set: "set", key: "key"}, function(err, result) {
  // The namespace does not exists
  if (!err)
    throw "error KeyExists: " + err;
});

console.log("client.KeyRemove");
client.KeyRemove({ns: "test", set: "set", key: "key"}, function(err) {
});

console.log("client.KeyExists");
client.KeyExists({ns: "test", set: "set", key: "key"}, function(err, result) {
  if (err)
    throw "error KeyExists: " + err;
  if (result)
    throw "error KeyExists: found!";
});


console.log("client.KeyExists");
client.KeyExists({ns: "test", set: "set", key: "key"}, function(err, result) {
  if (err)
    throw "error KeyExists: " + err;
  if (!result)
    throw "error KeyExists: not found!";
});

console.log("client.KeyGet");
client.KeyGet({ns: "test", set: "set", key: "key"}, [],
  function(err, result) {
    console.log("KeyGet-All", result);
    if (err)
      throw "error";
});

console.log("client.KeyGet");
client.KeyGet({ns: "test", set: "set", key: "key"},
  ["col2new", "col3new", "col4nonexists"],
  function(err, result) {
    console.log("KeyGet", result);
    console.log(err);
    if (err)
      throw "error";
});

console.log('END TEST');
*/