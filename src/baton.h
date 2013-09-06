#ifndef SRC_BATON_H
#define SRC_BATON_H

namespace nodejs_aerospike {

struct BatonBase {
  uv_work_t request;
  v8::Persistent<Function> callback;

  Client *client;
  as_status status; // AEROSPIKE_OK means no error
  as_error  error;
};

struct BatonKeyExists: public BatonBase {
  as_key key;
  bool result;
};

}

#endif // SRC_BATON_H
