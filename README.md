nodejs-aerospike
================

A NodeJS client for Aerospike 3 NoSQL DB.

Features
--------

* Connect to 

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
client.KeyPut({ns: "test", set: "set", key: "__TEST_KEY_OK__"}, 
    {col1new: "value1-new", col2new: "value2-new", col3new: 3},
    function(err) {
        assert.equal(err, undefined, "failed put-ok");
});
```

### Get a key

#### all records (=columns)
```js
client.KeyGet({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
    [],
    function(err, result) {
        assert.equal(err, undefined, "failed get-ok");
        assert.deepEqual(result, {col1new: "value1-new", col2new: "value2-new", col3new: 3});
});
```

#### specific records

```js
client.KeyGet({ns: "test", set: "set", key: "__TEST_KEY_OK__"},
    ["col1new", "col3new"],
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

