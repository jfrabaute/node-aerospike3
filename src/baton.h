#ifndef SRC_BATON_H
#define SRC_BATON_H

namespace nodejs_aerospike {

template<class D>
struct BatonBase: public D
{
  BatonBase() = delete;
  BatonBase(const BatonBase &) = delete;
  BatonBase(const BatonBase &&) = delete;
  BatonBase(Client *_client, Local<Function> &_cb)
  {
    client = _client;
    if (!_cb.IsEmpty())
      callback = Persistent<Function>::New(_cb);
    request.data = this;
  }

  virtual ~BatonBase()
  {
    callback.Dispose();
  }
public:
  uv_work_t request;
  v8::Persistent<Function> callback;

  Client *client;
  as_status status; // AEROSPIKE_OK means no error
  as_error  error;

};

struct DataNull
{
};
typedef BatonBase<DataNull> BatonNull;

struct DataKey
{
  as_key key;
};
typedef BatonBase<DataKey> BatonKey;

struct DataKeyExists
{
  as_key key;
  bool   result;
};
typedef BatonBase<DataKeyExists> BatonKeyExists;

struct DataKeyPut
{
  as_key    key;
  as_record record;
};
typedef BatonBase<DataKeyPut> BatonKeyPut;

struct DataKeyGet
{
  as_key     key;
  as_record *record;
  char     **bins;
  size_t     bins_size;

  ~DataKeyGet() {
    if (bins_size != 0) {
      for (size_t i = 0 ; i < bins_size ; ++i)
      {
        if (bins[i] == NULL)
          break;
        free(bins[i]);
      }
      free(bins);
    }
  }
};

typedef BatonBase<DataKeyGet> BatonKeyGet;

}

#endif // SRC_BATON_H
