#ifndef SRC_AEROSPIKE_ERROR_H
#define SRC_AEROSPIKE_ERROR_H

extern "C" {
  #include <aerospike/aerospike.h>
}
namespace nodejs_aerospike {

using namespace v8;

class Error: public node::ObjectWrap {
public:
  static void Init(Handle<Object> target);

private:
  Error();
  ~Error();

  static Handle<Value> New(const Arguments& args);

// object JS Methods
private:

// Internal object methods
private:

// Internal object properties
private:
  as_error error;
};

} // namespace nodejs_aerospike

#endif // SRC_AEROSPIKE_ERROR_H
