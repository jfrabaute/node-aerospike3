
#include <node.h>
#include <string.h>
extern "C" {
  #include <aerospike/as_key.h>
  #include <aerospike/aerospike_key.h>
  #include <aerospike/as_record_iterator.h>
}

#include "aerospike.h"
#include "aerospike_error.h"

using namespace v8;

namespace nodejs_aerospike {

Error::Error()
{
}

Error::~Error()
{
}

void Error::Init(Handle<Object> target)
{
  MY_NODE_ISOLATE_DECL

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(MY_NODE_ISOLATE_PRE New);
  tpl->SetClassName(String::NewSymbol("Error"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Get prototype
  Handle<ObjectTemplate> proto = tpl->PrototypeTemplate();

  // Add methods

  Persistent<Function> constructor = Persistent<Function>::New(MY_NODE_ISOLATE_PRE tpl->GetFunction());
  target->Set(String::NewSymbol("Error"), constructor);
}

Handle<Value> Error::New(const Arguments& args)
{
  Error* obj = new Error();
  obj->Wrap(args.Holder());

  return args.Holder();
}

} // namespace nodejs_aerospike
