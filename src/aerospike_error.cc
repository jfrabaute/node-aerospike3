
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

Persistent<ObjectTemplate> Error::error_templ;

void Error::Init(Handle<Object> target)
{
  MY_NODE_ISOLATE_DECL

  error_templ = Persistent<ObjectTemplate>::New(ObjectTemplate::New());
  error_templ->SetInternalFieldCount(1);
  error_templ->SetAccessor(String::New("code"), GetCode);
  error_templ->SetAccessor(String::New("message"), GetMessage);
  error_templ->SetAccessor(String::New("func"), GetFunc);
  error_templ->SetAccessor(String::New("file"), GetFile);
  error_templ->SetAccessor(String::New("line"), GetLine);
}

Local<Object> Error::NewInstance(std::unique_ptr<as_error> e)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Error* error = new Error(std::move(e));
  Persistent<Object> obj = Persistent<Object>::New(error_templ->NewInstance());
  obj->SetInternalField(0, External::New(error));
  obj.MakeWeak(error, FreeInstance);

  return scope.Close(obj);
}

void Error::FreeInstance(Persistent<Value> object, void *param)
{
  Error* error = static_cast<Error*>(param);
  delete error;

  object.Dispose();
  object.Clear();
}

Handle<Value> Error::GetCode(Local<String> property, const AccessorInfo &info)
{
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  void* ptr = wrap->Value();
  int value = static_cast<Error*>(ptr)->error->code;
  return Integer::New(value);
}

Handle<Value> Error::GetMessage(Local<String> property, const AccessorInfo &info)
{
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  void* ptr = wrap->Value();
  char* value = static_cast<Error*>(ptr)->error->message;
  if (value != NULL)
    return String::New(value);
  return String::New("--Not Provided--");
}

Handle<Value> Error::GetFunc(Local<String> property, const AccessorInfo &info)
{
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  void* ptr = wrap->Value();
  const char* value = static_cast<Error*>(ptr)->error->func;
  if (value != NULL)
    return String::New(value);
  return String::New("--Not Provided--");
}

Handle<Value> Error::GetFile(Local<String> property, const AccessorInfo &info)
{
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  void* ptr = wrap->Value();
  const char* value = static_cast<Error*>(ptr)->error->file;
  if (value != NULL)
    return String::New(value);
  return String::New("--Not Provided--");
}

Handle<Value> Error::GetLine(Local<String> property, const AccessorInfo &info)
{
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  void* ptr = wrap->Value();
  uint32_t value = static_cast<Error*>(ptr)->error->line;
  return Integer::New(value);
}

} // namespace nodejs_aerospike
