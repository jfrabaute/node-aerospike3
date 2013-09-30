nodejs-aerospike
================

A NodeJS client for Aerospike 3 NoSQL DB.

Features
--------

* Connect to an aerospike cluster
* Key operations
** Put
** Exists
** Get
** Remove
** Operate

Install
-------

TODO

Requirements
------------

### Binary use
* (http://www.aerospike.com/aerospike-3-client-sdk/ "aerospike 3 C Client").
* NodeJS 0.10

### Binary compile

* node-gyp module
* g++

### Run the tests
* async nodejs module

Usage
-----

### Create a client and connect to the DB:

```js
var aerospike = require(''),
    client = new aerospike.Client();

client.Connect({}, function(err) {
    if (err) throw err;
    if (client.IsConnected() != true)
        throw "Should be true";
});
```

#### Connection options

The first argument of the Connect method is:

* an object with "host(=127.0.0.1)", "port(=3000)" as optional properties.
* an array of objects defining a host (see first point)

### Put a key

```js
client.KeyPut({
	key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
    	record: {col1new: "value1-new", col2new: "value2-new", col3new: 3}
    },
    function(err) {
        assert.equal(err, undefined, "failed put-ok");
});
```

### Get a key

#### all records (=columns)
```js
client.KeyGet({key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"}},
    function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
        assert.deepEqual(result, {col1new: "value1-new", col2new: "value2-new", col3new: 3});
});
```

#### specific records

```js
client.KeyGet({
	key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
	record: ["col1new", "col3new"]
    },
    function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
        assert.deepEqual(result, {col1new: "value1-new", col3new: 3});
});
```

### Check if a key exists


```js
client.KeyExists({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
    function(err, result) {
        assert.equal(err, undefined, "failed exists-ok");
        assert.equal(result, true);
});
```

### Operate on a key

```js
client.KeyOperate({
	key: {ns: "test", set: "set", key: "__TEST_KEY_OK__"},
	ops: [
		{op: "append_str", col: "bin1", value: "_post"}, // Append to an existing column string
		{op: "incr", col: "bin1", value: 12}, // Increment the value of a column
		{op: "prepend_str", col: "bin1", value: "_post"}, // Prepend to an existing column string
		{op: "read", col: "bin1"}, // Read a value from a bin. This is ideal, if you performed an operation on a bin, and want to read the new value.
		{op: "touch"} // Touching a record will refresh its ttl and increment the generation of the record.
		{op: "write_str", col: "bin3", value: "write"} // write a string
		{op: "write_int64", col: "bin3", value: 123} // write an int64
	]
	},
    function(err, result) {
        assert.equal(err, undefined, "failed operate-ok");
});
```


