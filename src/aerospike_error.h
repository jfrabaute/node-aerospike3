#ifndef SRC_AEROSPIKE_ERROR_H
#define SRC_AEROSPIKE_ERROR_H

#include <memory>

extern "C" {
  #include <aerospike/aerospike.h>
}
namespace nodejs_aerospike {

using namespace v8;

class Error
{
public:
  static void Init(Handle<Object> target);

  Error() = delete;
  Error(Error&) = delete;
  Error(Error&&) = delete;

  static Local<Object> NewInstance(std::unique_ptr<as_error> e);

private:
  Error(std::unique_ptr<as_error> e)
    : error(std::move(e))
  {}

  static Persistent<ObjectTemplate> error_templ;

  static void FreeInstance(Persistent<Value> object, void *param);

// object JS Methods
private:
  static Handle<Value> GetCode(Local<String> property, const AccessorInfo &info);
  static Handle<Value> GetMessage(Local<String> property, const AccessorInfo &info);
  static Handle<Value> GetFunc(Local<String> property, const AccessorInfo &info);
  static Handle<Value> GetFile(Local<String> property, const AccessorInfo &info);
  static Handle<Value> GetLine(Local<String> property, const AccessorInfo &info);


// Internal object methods
private:

// Internal object properties
private:
  std::unique_ptr<as_error> error;
};

} // namespace nodejs_aerospike

#endif // SRC_AEROSPIKE_ERROR_H
