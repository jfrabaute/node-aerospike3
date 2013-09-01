
#include <node.h>
#include <v8.h>

#include "aerospike.h"
#include "aerospike_client.h"

void InitAerospike(v8::Handle<v8::Object> target) {
  nodejs_aerospike::Client::Init(target);
}

NODE_MODULE(aerospike, InitAerospike)
