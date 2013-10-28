var aerospike_cpp,
    util = require('util'),
    events = require('events');

try {
  aerospike_cpp = require('./build/Debug/aerospike_cpp');
} catch (error) {
  aerospike_cpp = require('./build/Release/aerospike_cpp');
}

function AsClient (hosts) {
  var self = this;

  this._client = aerospike_cpp.createClient();
  hosts = hosts || {};
  hosts.host = hosts.host || "127.0.0.1";
  hosts.port = hosts.port || 3000;
  if (!util.isArray(hosts))
    hosts = [hosts];

  this._hosts = hosts;

  this._queue = [];

  this._client.connect(hosts, function(err) {
    if (err) {
      self.emit('error', err);
    } else {
      self.emit('connect');
    }
    self._processQueue();
  });

  events.EventEmitter.call(this);
}

util.inherits(AsClient, events.EventEmitter)

AsClient.prototype.isConnected = function() {
  return this._client.isConnected();
}

AsClient.prototype.isConnecting = function() {
  return this._client.isConnecting();
}

function createCallback(self, func, c) {
  return function(err, result) {
    if (c) {
      c(err, result);
    }
    if (func === 'close') {
      // Return a "not connected" error
      // for all the pending operations in the queue
      self._processQueue();
      self.emit('close', err);
    }
  }
}

AsClient.prototype._processQueue = function() {
  var op;
  if (this._client.isConnecting())
    return;
  if (!this._client.isConnected()) {
    // Return a "not connected" error
    // for all the pending operations in the queue
    var err = {
      code: -1,
      message: 'Aerospike client not connected',
    };
    while (this._queue.length) {
      op = this._queue.shift();
      if (op.args[1]) {
        op.args[1].call(this._client, err);
      }
    }
  } else {
    // Run all the pending operations in the queue
    while (this._queue.length) {
      op = this._queue.shift();
      this._client[op.funcName].call(this._client, op.args[0], createCallback(this, op.funcName, op.args[1]));
    }
  }
}

var asyncFuncs = [
  'keyExists',
  'keyPut',
  'keyGet',
  'keyRemove',
  'keyOperate',
  'close'
];

function createMethod(name) {
  return function() {
    // Add it to the queue
    this._queue.push({funcName: name, args: arguments});
    // Process the queue
    this._processQueue();
  }
}

for (func in asyncFuncs) {
  AsClient.prototype[asyncFuncs[func]] = createMethod(asyncFuncs[func]);
}

module.exports.createClient = function(hosts) {
  return new AsClient(hosts);
}

