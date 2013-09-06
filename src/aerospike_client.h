#ifndef SRC_AEROSPIKE_CLIENT_H
#define SRC_AEROSPIKE_CLIENT_H

extern "C" {
  #include <aerospike/aerospike.h>
}
namespace nodejs_aerospike {

using namespace v8;

class Client: public node::ObjectWrap {
public:
  static void Init(Handle<Object> target);

private:
  Client();
  ~Client();

  static Handle<Value> New(const Arguments& args);

// object JS Methods
private:
  static Handle<Value> Connect(const Arguments& args);
  static Handle<Value> IsConnected(const Arguments& args);
  static Handle<Value> Close(const Arguments& args);

  static Handle<Value> KeyExists(const Arguments& args);
  static Handle<Value> KeyPut(const Arguments& args);
  static Handle<Value> KeyGet(const Arguments& args);
  static Handle<Value> KeyRemove(const Arguments& args);

// Internal object methods
private:
  void close();

// Internal object properties
private:
  as_config config;
  as_error  err; // TODO: remove this property
  aerospike as;
  bool      initialized;
  bool      connected;
};

} // namespace nodejs_aerospike

#endif // SRC_AEROSPIKE_CLIENT_H
