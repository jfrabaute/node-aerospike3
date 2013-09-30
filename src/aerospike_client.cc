
#include <node.h>
#include <string.h>

extern "C" {
  #include <aerospike/as_key.h>
  #include <aerospike/aerospike_key.h>
  #include <aerospike/as_record_iterator.h>
}

#include "aerospike.h"
#include "aerospike_client.h"
#include "aerospike_error.h"
#include "baton.h"
#include "helper.h"

using namespace v8;
using namespace nodejs_aerospike::helper;

namespace nodejs_aerospike {

namespace {

  bool initConfigEntry(const Handle<Object> &object, uint32_t id, as_config &config)
  {
    Handle<Value> port = object->Get(String::New("port"));
    if (port->IsUndefined())
    {
      config.hosts[id].port = 3000;
    }
    else if (port->IsUint32())
    {
      config.hosts[id].port = port->Uint32Value();
    }
    else
    {
      ThrowException(Exception::TypeError(String::New("\"port\" property is not a number in config object")));
      return false;
    }

    Handle<Value> addrJs = object->Get(String::New("host"));
    if (addrJs->IsUndefined())
    {
        config.hosts[id].addr = strdup("127.0.0.1");
    }
    else if (addrJs->IsString())
    {
      String::AsciiValue addr(addrJs);
      if (addr.length() == 0)
      {
        ThrowException(Exception::TypeError(String::New("\"host\" property could not be converted into a valid ascii string")));
        return false;
      }
      config.hosts[id].addr = strdup(*addr);
      if (config.hosts[id].addr == NULL)
      {
        ThrowException(Exception::TypeError(String::New("Not Enough Memory: Unable to copy the addr data")));
        return false;
      }
    }
    else
    {
      ThrowException(Exception::TypeError(String::New("\"host\" property is not a string in config object")));
      return false;
    }

    return true;
  }


} // unamed namespace


bool Client::connected = false;
bool Client::connecting = false;

Client::Client()
  : nb_hosts(0)
{
}

Client::~Client()
{
  as_error error;
  close(error);
}

void Client::Init(Handle<Object> target)
{
  MY_NODE_ISOLATE_DECL

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(MY_NODE_ISOLATE_PRE New);
  tpl->SetClassName(String::NewSymbol("Client"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Get prototype
  Handle<ObjectTemplate> proto = tpl->PrototypeTemplate();

  // Add methods
  proto->Set(String::NewSymbol("Connect"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE Connect)->GetFunction() );
  proto->Set(String::NewSymbol("IsConnected"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE IsConnected)->GetFunction() );
  proto->Set(String::NewSymbol("Close"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE Close)->GetFunction()   );

  proto->Set(String::NewSymbol("KeyExists"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE KeyExists)->GetFunction()   );
  proto->Set(String::NewSymbol("KeyPut"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE KeyPut)->GetFunction()   );
  proto->Set(String::NewSymbol("KeyGet"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE KeyGet)->GetFunction()   );
  proto->Set(String::NewSymbol("KeyRemove"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE KeyRemove)->GetFunction()   );
  proto->Set(String::NewSymbol("KeyOperate"), FunctionTemplate::New(MY_NODE_ISOLATE_PRE KeyOperate)->GetFunction()   );

  Persistent<Function> constructor = Persistent<Function>::New(MY_NODE_ISOLATE_PRE tpl->GetFunction());
  target->Set(String::NewSymbol("Client"), constructor);
}

Handle<Value> Client::New(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  if (!args.IsConstructCall())
  {
    return ThrowException(Exception::TypeError(String::New("Use the new operator to create new Client objects")));
  }

  if (Client::connected || Client::connecting)
  {
    return ThrowException(Exception::TypeError(String::New("Aerospike C Client limitation: Applications that use this client can only connect to a single cluster at a time. This will be fixed in a future release.")));
  }

  Client* obj = new Client();
  obj->Wrap(args.Holder());

  return args.Holder();
}

Handle<Value> Client::Connect(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (client->connected)
  {
    ThrowException(Exception::TypeError(String::New("Client already connected.")));
    return scope.Close(Undefined());
  }

  if (client->connecting)
  {
    ThrowException(Exception::TypeError(String::New("Client already connecting.")));
    return scope.Close(Undefined());
  }

  client->initAsConfig();

  // Read the param
  if (args.Length() != 2)
  {
    ThrowException(Exception::TypeError(String::New("Invalid number of arguments.")));
    return scope.Close(Undefined());
  }

  Local<Function> cb;

  if (!args[1]->IsFunction())
  {
    ThrowException(Exception::TypeError(String::New("Wrong second argument type (must be callback function)")));
    return scope.Close(Undefined());
  }
  cb = Local<Function>::Cast(args[1]);

  if (args[0]->IsArray())
  {
    Handle<Array> array = Handle<Array>::Cast(args[0]);
    if (array->Length() > AS_CONFIG_HOSTS_SIZE)
    {
      ThrowException(Exception::TypeError(String::New("Too many hosts provided: The maximum number of possible host is 256.")));
      return scope.Close(Undefined());
    }
    for (uint32_t i = 0 ; i < array->Length() ; ++i)
    {
      Handle<Value> object =array->Get(i);
      if (!object->IsObject())
      {
        ThrowException(Exception::TypeError(String::New("Wrong array entry (must be an object)")));
        return scope.Close(Undefined());
      }
      if (!initConfigEntry(object->ToObject(), i, client->config))
        return scope.Close(Undefined());
      client->nb_hosts++;
    }
  }
  else if (args[0]->IsObject())
  {
    if (!initConfigEntry(Handle<Object>::Cast(args[0]), 0, client->config))
      return scope.Close(Undefined());
    client->nb_hosts++;
  }
  else
  {
    ThrowException(Exception::TypeError(String::New("Wrong first argument type (must be either object or array of objects)")));
    return scope.Close(Undefined());
  }

  if (client->nb_hosts == 0)
  {
    ThrowException(Exception::TypeError(String::New("No valid host provided")));
    return scope.Close(Undefined());
  }

  // Run connect asynchronously
  BatonNull* baton = new BatonNull(client, cb);

  Client::connecting = true;

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonNull* baton = static_cast<BatonNull*>(req->data);
                  aerospike_init(&baton->client->as, &baton->client->config);
                  if (aerospike_connect(&baton->client->as, baton->error.get()) != AEROSPIKE_OK)
                    aerospike_destroy(&baton->client->as);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  Client::connecting = false;

                  BatonNull* baton = static_cast<BatonNull*>(req->data);

                  const unsigned argc = 1;
                  Handle<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    baton->client->connected = true;
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                  }

                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

Handle<Value> Client::IsConnected(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  return scope.Close(client->connected ? True() : False());
}

Handle<Value> Client::Close(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  // Read the callback if present
  if (args.Length() > 1)
  {
    ThrowException(Exception::TypeError(String::New("Invalid number of arguments.")));
    return scope.Close(Undefined());
  }

  Local<Function> cb;
  if (args.Length() == 1)
  {
    if (!args[0]->IsFunction())
    {
      ThrowException(Exception::TypeError(String::New("Wrong first argument type (must be callback function)")));
      return scope.Close(Undefined());
    }
    cb = Local<Function>::Cast(args[0]);
  }

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (!client->connected)
  {
    ThrowException(Exception::TypeError(String::New("Client not connected")));
    return scope.Close(Undefined());
  }

  // Run close asynchronously
  BatonNull* baton = new BatonNull(client, cb);

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonNull* baton = static_cast<BatonNull*>(req->data);
                  baton->client->close(*baton->error.get());
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  BatonNull* baton = static_cast<BatonNull*>(req->data);

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                  }
                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );


  return scope.Close(Undefined());
}

as_status Client::close(as_error &err)
{
  as_status result = AEROSPIKE_OK;
  if (connected)
  {
    result = aerospike_close(&as, &err);
    aerospike_destroy(&as);
    connected = false;
  }

  initAsConfig();

  return result;
}

void Client::initAsConfig()
{
  for (uint8_t i = 0 ; i < nb_hosts ; i++)
  {
    free(const_cast<char*>(config.hosts[i].addr));
  }
  nb_hosts = 0;

  as_config_init(&config);
}

Handle<Value> Client::KeyExists(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (!client->connected)
  {
    ThrowException(Exception::Error(String::New("Client is not connected.")));
    return scope.Close(Undefined());
  }
  if (args.Length() < 2)
  {
    ThrowException(Exception::TypeError(String::New("Missing arguments")));
    return scope.Close(Undefined());
  }
  if (!args[1]->IsFunction())
  {
    ThrowException(Exception::TypeError(String::New("Second argument is not a function")));
    return scope.Close(Undefined());
  }

  Local<Function> cb(Local<Function>::Cast(args[1]));
  BatonKeyExists *baton = new BatonKeyExists(client, cb);

  if (!Helper::GetKeyFromArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKeyExists* baton = static_cast<BatonKeyExists*>(req->data);
                  aerospike_key_exists(&baton->client->as, baton->error.get(), NULL, &baton->key, &baton->result);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  BatonKeyExists* baton = static_cast<BatonKeyExists*>(req->data);

                  const unsigned argc = 2;
                  Local<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                    argv[1] = Local<Value>::New(baton->result ? True() : False());
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                    argv[1] = Local<Value>::New(Undefined());
                  }
                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

Handle<Value> Client::KeyPut(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (!client->connected)
  {
    ThrowException(Exception::Error(String::New("Client is not connected.")));
    return scope.Close(Undefined());
  }
  if (!args[1]->IsFunction())
  {
    ThrowException(Exception::TypeError(String::New("Second argument is not a callback function")));
    return scope.Close(Undefined());
  }

  Local<Function> cb(Local<Function>::Cast(args[1]));
  BatonKeyPut *baton = new BatonKeyPut(client, cb);

  if (!Helper::GetKeyFromKeyBinsArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  if (!Helper::GetRecordFromKeyRecordArg(args[0], baton->record))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKeyPut* baton = static_cast<BatonKeyPut*>(req->data);
                  aerospike_key_put(&baton->client->as, baton->error.get(), NULL, &baton->key, &baton->record);
                  as_record_destroy(&baton->record);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  BatonKeyPut* baton = static_cast<BatonKeyPut*>(req->data);

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                  }
                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

Handle<Value> Client::KeyGet(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (args.Length() < 2)
  {
    ThrowException(Exception::TypeError(String::New("Missing arguments")));
    return scope.Close(Undefined());
  }
  if (!args[1]->IsFunction())
  {
    ThrowException(Exception::TypeError(String::New("Second argument is not a function")));
    return scope.Close(Undefined());
  }

  Local<Function> cb(Local<Function>::Cast(args[1]));
  BatonKeyGet *baton = new BatonKeyGet(client, cb);

  if (!Helper::GetKeyFromKeyBinsArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  Handle<Object> obj = Handle<Object>::Cast(args[0]);

  Handle<Value> bins = obj->Get(String::New("record"));

  if (bins->IsUndefined())
  {
    // Return all the bins
    baton->bins = NULL;
    baton->bins_size = 0;
  }
  else if (bins->IsArray())
  {
    Handle<Array> array = Handle<Array>::Cast(bins);

    if (array->Length() == 0)
    {
      // Return all the bins
      baton->bins = NULL;
      baton->bins_size = 0;
    }
    else
    {
      // Return only requested bins
      baton->bins_size = array->Length() + 1;
      baton->bins = (char**)malloc(baton->bins_size);
      memset(baton->bins, '\0', sizeof(char*)*baton->bins_size);
      for (uint32_t i = 0 ; i < baton->bins_size-1 ; ++i)
      {
        String::AsciiValue bin_str(array->Get(i)->ToString());
        if (bin_str.length() == 0)
        {
          ThrowException(Exception::TypeError(String::New("\"record\" property could not be converted into a valid ascii string")));
          delete baton;
          return scope.Close(Undefined());
        }
        else if (bin_str.length() > AS_BIN_NAME_MAX_LEN)
        {
          ThrowException(Exception::TypeError(String::New("one record property is too big (15 characters max)")));
          delete baton;
          return scope.Close(Undefined());
        }
        baton->bins[i] = strdup(*bin_str);
      }
    }
  }
  else
  {
    ThrowException(Exception::TypeError(String::New("bins property should be an array.")));
    return scope.Close(Undefined());
  }

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKeyGet* baton = static_cast<BatonKeyGet*>(req->data);
                  if (baton->bins == NULL)
                  {
                    // Return all the bins
                    aerospike_key_get(&baton->client->as, baton->error.get(), NULL, &baton->key, &baton->record);
                  }
                  else
                  {
                    // Return only requested bins
                    aerospike_key_select(&baton->client->as, baton->error.get(), NULL, &baton->key, const_cast<const char**>(baton->bins), &baton->record);
                  }
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  BatonKeyGet* baton = static_cast<BatonKeyGet*>(req->data);

                  const unsigned argc = 2;
                  Local<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                    // Transform the ac_record into a js object
                    argv[1] = Object::New();

                    as_record_iterator it;
                    as_record_iterator_init(&it, baton->record);

                    bool error = false;
                    while ( as_record_iterator_has_next(&it) )
                    {
                      as_bin * bin = as_record_iterator_next(&it);
                      as_val * value = (as_val *) as_bin_get_value(bin);
                      switch ( as_val_type(value) ) {
                        case AS_NIL:
                          Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), Null());
                          break;
                        case AS_INTEGER:
                          Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), Number::New(as_record_get_int64 (baton->record, bin->name, 0)));
                          break;
                        case AS_STRING:
                          Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), String::New(as_string_get(as_string_fromval(value))));
                          break;
                        case AS_UNDEF:
                          Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), Undefined());
                          break;
                        case AS_BYTES:
                        case AS_LIST:
                        case AS_MAP:
                        case AS_REC:
                        default:
                          argv[0] = Local<Value>::New(String::New("Type not supported"));
                          argv[1] = Local<Value>::New(Undefined());
                          error = true;
                          break;
                      }
                      if (error)
                        break;
                    }
                    as_record_iterator_destroy(&it);

                    as_record_destroy(baton->record);
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                    argv[1] = Local<Value>::New(Undefined());
                  }
                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

Handle<Value> Client::KeyRemove(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (!client->connected)
  {
    ThrowException(Exception::Error(String::New("Client is not connected.")));
    return scope.Close(Undefined());
  }
  if (args.Length() < 2)
  {
    ThrowException(Exception::TypeError(String::New("Missing arguments")));
    return scope.Close(Undefined());
  }
  if (!args[1]->IsFunction())
  {
    ThrowException(Exception::TypeError(String::New("Second argument is not a callback function")));
    return scope.Close(Undefined());
  }

  Local<Function> cb(Local<Function>::Cast(args[1]));
  BatonKey *baton = new BatonKey(client, cb);

  if (!Helper::GetKeyFromArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKey* baton = static_cast<BatonKey*>(req->data);
                  aerospike_key_remove(&baton->client->as, baton->error.get(), NULL, &baton->key);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  BatonKey* baton = static_cast<BatonKey*>(req->data);

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                  }
                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

Handle<Value> Client::KeyOperate(const Arguments& args)
{
  MY_NODE_ISOLATE_DECL
  MY_HANDLESCOPE

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  if (args.Length() < 2)
  {
    ThrowException(Exception::TypeError(String::New("Missing arguments")));
    return scope.Close(Undefined());
  }
  if (!args[0]->IsObject())
  {
    ThrowException(Exception::TypeError(String::New("First argument is not an object")));
    return scope.Close(Undefined());
  }
  if (!args[1]->IsFunction())
  {
    ThrowException(Exception::TypeError(String::New("Second argument is not a function")));
    return scope.Close(Undefined());
  }

  Local<Function> cb(Local<Function>::Cast(args[1]));
  BatonKeyOperate *baton = new BatonKeyOperate(client, cb);

  if (!Helper::PopulateOps(args[0]->ToObject(), baton))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKeyOperate* baton = static_cast<BatonKeyOperate*>(req->data);
                  // Return only requested bins
                  aerospike_key_operate(&baton->client->as, baton->error.get(), NULL, &baton->key, &baton->ops, &baton->record);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  MY_NODE_ISOLATE_DECL
                  MY_HANDLESCOPE

                  BatonKeyOperate* baton = static_cast<BatonKeyOperate*>(req->data);

                  const unsigned argc = 2;
                  Local<Value> argv[argc];
                  if (baton->error->code == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                    // Transform the ac_record into a js object
                    argv[1] = Object::New();

                    if (baton->record != NULL)
                    {
                      as_record_iterator it;
                      as_record_iterator_init(&it, baton->record);

                      bool error = false;
                      while ( as_record_iterator_has_next(&it) )
                      {
                        as_bin * bin = as_record_iterator_next(&it);
                        as_val * value = (as_val *) as_bin_get_value(bin);
                        switch ( as_val_type(value) ) {
                          case AS_NIL:
                            Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), Null());
                            break;
                          case AS_INTEGER:
                            Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), Number::New(as_record_get_int64 (baton->record, bin->name, 0)));
                            break;
                          case AS_STRING:
                            Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), String::New(as_string_get(as_string_fromval(value))));
                            break;
                          case AS_UNDEF:
                            Local<Object>::Cast(argv[1])->Set(String::NewSymbol(as_bin_get_name(bin)), Undefined());
                            break;
                          case AS_BYTES:
                          case AS_LIST:
                          case AS_MAP:
                          case AS_REC:
                          default:
                            argv[0] = Local<Value>::New(String::New("Type not supported"));
                            argv[1] = Local<Value>::New(Undefined());
                            error = true;
                            break;
                        }
                        if (error)
                          break;
                      }
                      as_record_iterator_destroy(&it);

                      as_record_destroy(baton->record);
                    }
                  }
                  else
                  {
                    argv[0] = ERRORNEWINSTANCE(baton->error);
                    argv[1] = Local<Value>::New(Undefined());
                  }
                  TryCatch try_catch;
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                  }
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

} // namespace nodejs_aerospike
