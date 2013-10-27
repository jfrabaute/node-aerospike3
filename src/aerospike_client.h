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

  void AddRef() { Ref(); }
  void Release() { Unref(); }

  static Handle<v8::Value> NewInstance(const Arguments& args);

private:
  Client();
  ~Client();

  static Handle<Value> New(const Arguments& args);

// object JS Methods
private:
  static Handle<Value> Connect(const Arguments& args);
  static Handle<Value> IsConnected(const Arguments& args);
  static Handle<Value> IsConnecting(const Arguments& args);
  static Handle<Value> Close(const Arguments& args);

  static Handle<Value> KeyExists(const Arguments& args);
  static Handle<Value> KeyPut(const Arguments& args);
  static Handle<Value> KeyGet(const Arguments& args);
  static Handle<Value> KeyRemove(const Arguments& args);
  static Handle<Value> KeyOperate(const Arguments& args);

// Internal object methods
private:
  as_status close(as_error &err);

  void initAsConfig();

  static Persistent<Function> client_constructor;

// Internal object properties
private:
  as_config config;
  aerospike as;

  uint8_t nb_hosts;

  // This should not be static, but we allow only one client at a time right now
  static bool connected;
  static bool connecting;
};

} // namespace nodejs_aerospike

#endif // SRC_AEROSPIKE_CLIENT_H
