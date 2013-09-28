var aerospike;

try {
  aerospike = require('./build/Debug/aerospike');
} catch (error) {
  aerospike = require('./build/Release/aerospike');
}

module.exports = aerospike;

