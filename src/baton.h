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

struct BatonKey: public BatonBase {
  as_key key;
};

struct BatonKeyExists: public BatonKey {
  bool result;
};

struct BatonKeyPut: public BatonKey {
  as_record record;
};

struct BatonKeyGet: public BatonKey {
  as_record *record;
  char **bins;
  size_t bins_size;

  ~BatonKeyGet() {
    if (bins_size != 0) {
      for (size_t i = 0 ; i < bins_size ; ++i)
        if (bins[i] != NULL)
          free(bins[i]);
        else
          break;
      free(bins);
    }
  }
};

}

#endif // SRC_BATON_H
