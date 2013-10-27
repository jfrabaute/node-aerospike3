
#include <node.h>
#include <v8.h>

#include "aerospike.h"
#include "aerospike_client.h"
#include "aerospike_error.h"

using namespace v8;

namespace nodejs_aerospike {

Handle<Value> CreateClient(const Arguments& args) {
  HandleScope scope;
  return scope.Close(Client::NewInstance(args));
}

void InitAerospike(v8::Handle<v8::Object> target) {
  nodejs_aerospike::Client::Init(target);
  nodejs_aerospike::Error::Init(target);

  // Add "createClient" method
  target->Set(String::NewSymbol("createClient"),
    FunctionTemplate::New(CreateClient)->GetFunction());
}

} // namespace nodejs_aerospike

NODE_MODULE(aerospike_cpp, nodejs_aerospike::InitAerospike)
