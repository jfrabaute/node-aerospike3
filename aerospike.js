var aerospike;

try {
  aerospike = require('./build/Debug/aerospike_cpp');
} catch (error) {
    console.log(error);
  aerospike = require('./build/Release/aerospike_cpp');
}

module.exports = aerospike;

