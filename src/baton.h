#ifndef SRC_BATON_H
#define SRC_BATON_H

#include <vector>
#include <string>

namespace nodejs_aerospike {

#define ERRORNEWINSTANCE(error) Error::NewInstance(std::move(error))

template<class D>
struct BatonBase: public D
{
  BatonBase() = delete;
  BatonBase(const BatonBase &) = delete;
  BatonBase(const BatonBase &&) = delete;
  BatonBase(Client *_client, Local<Function> &_cb)
    : client(_client)
    , error(new as_error)
  {
    client->AddRef();
    if (!_cb.IsEmpty())
      callback = Persistent<Function>::New(_cb);
    request.data = this;
  }

  virtual ~BatonBase()
  {
    callback.Dispose();
    client->Release();
  }

  uv_work_t request;
  v8::Persistent<Function> callback;

  Client *client;
  std::unique_ptr<as_error> error;

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

  DataKeyGet()
    : record(NULL)
  {
  }

  ~DataKeyGet()
  {
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

struct DataKeyOperate
{
  as_key        key;
  as_operations ops;
  as_record *record;

  DataKeyOperate()
    : record(NULL)
    , destroy(false)
  {
  }

  ~DataKeyOperate()
  {
    if (record != NULL)
      as_record_destroy(record);
    if (destroy)
      as_operations_destroy(&ops);
  }

  bool destroy; // set to true if destroy needs to be called in the destructor

  std::vector<std::string> strings; // Strings allocated for the data, that will be
                                    // automatically destroyed
};

typedef BatonBase<DataKeyOperate> BatonKeyOperate;

}

#endif // SRC_BATON_H
