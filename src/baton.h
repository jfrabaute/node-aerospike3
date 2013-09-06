#ifndef SRC_BATON_H
#define SRC_BATON_H

namespace nodejs_aerospike {

struct BatonConnect {
  uv_work_t request;
  v8::Persistent<Function> callback;

  Client *client;
  as_status status; // AEROSPIKE_OK means no error
  as_error  error;
};

}

#endif // SRC_BATON_H
