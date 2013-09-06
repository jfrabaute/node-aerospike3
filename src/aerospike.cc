
#include <node.h>
#include <v8.h>

#include "aerospike.h"
#include "aerospike_client.h"
#include "aerospike_error.h"

void InitAerospike(v8::Handle<v8::Object> target) {
  nodejs_aerospike::Client::Init(target);
  nodejs_aerospike::Error::Init(target);
}

NODE_MODULE(aerospike, InitAerospike)
