
#include <node.h>
#include <string.h>
extern "C" {
  #include <aerospike/as_key.h>
  #include <aerospike/aerospike_key.h>
  #include <aerospike/as_record_iterator.h>
}

#include "aerospike.h"
#include "aerospike_client.h"
#include "baton.h"

using namespace v8;

namespace nodejs_aerospike {

namespace {

  bool initConfigEntry(const Handle<Object> &object, uint32_t id, as_config &config)
  {
    Handle<Value> addrJs = object->Get(String::New("host"));
    Handle<Value> port = object->Get(String::New("port"));
    if (addrJs->IsUndefined())
    {
        config.hosts[id].addr = "127.0.0.1";
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

    return true;
  }

  bool getKeyFromObject(const Handle<Object> &arg, as_key &key)
  {
    Handle<Object> obj = Handle<Object>::Cast(arg);

    Handle<Value> ns = obj->Get(String::New("ns"));
    if (!ns->IsString())
    {
      ThrowException(Exception::TypeError(String::New("Missing or non string \"ns\" property in key object.")));
      return false;
    }
    String::AsciiValue ns_str(ns);
    if (ns_str.length() == 0)
    {
      ThrowException(Exception::TypeError(String::New("\"ns\" property could not be converted into a valid ascii string")));
      return false;
    }

    Handle<Value> set = obj->Get(String::New("set"));
    if (!set->IsString())
    {
      ThrowException(Exception::TypeError(String::New("Missing or non string \"set\" property in key object.")));
      return false;
    }
    String::AsciiValue set_str(set);
    if (set_str.length() == 0)
    {
      ThrowException(Exception::TypeError(String::New("\"set\" property could not be converted into a valid ascii string")));
      return false;
    }

    Handle<Value> key_name = obj->Get(String::New("key"));
    if (key_name->IsString())
    {
      String::AsciiValue key_str(key_name);
      if (key_str.length() == 0)
      {
        as_key_init_raw(&key, *ns_str, *set_str, reinterpret_cast<const uint8_t*>(*String::Utf8Value(key_name->ToString())), key_name->ToString()->Utf8Length());
      }
      else
      {
        as_key_init_str(&key, *ns_str, *set_str, strdup(*key_str));
      }
    }
    else if (key_name->IsNumber())
    {
      as_key_init_int64(&key, *ns_str, *set_str, key_name->ToNumber()->Value());
    }
    else
    {
      ThrowException(Exception::TypeError(String::New("Invalid or unsupported type for \"key\" property in key object.")));
      return false;
    }

    return true;
  }

  bool getKeyFromKeyBinsArg(const Handle<Value> &arg, as_key &key)
  {
    if (!arg->IsObject())
    {
      ThrowException(Exception::TypeError(String::New("Expected object for argument.")));
      return false;
    }

    Handle<Object> obj = Handle<Object>::Cast(arg);
    Handle<Value> keyArg = obj->Get(String::New("key"));
    if (!keyArg->IsObject())
    {
      ThrowException(Exception::TypeError(String::New("Expected object for key argument.")));
      return false;
    }

    return getKeyFromObject(Handle<Object>::Cast(keyArg), key);
  }

  bool getKeyFromArg(const Handle<Value> &arg, as_key &key)
  {
    if (!arg->IsObject())
    {
      ThrowException(Exception::TypeError(String::New("Expected object type for key argument.")));
      return false;
    }

    return getKeyFromObject(Handle<Object>::Cast(arg), key);
  }

  bool getRecordFromObject(const Handle<Object> &rec, as_record &record)
  {
    Local<Array> properties = rec->GetOwnPropertyNames();
    if (properties->Length() == 0)
    {
      ThrowException(Exception::TypeError(String::New("No values defined in the record.")));
      return false;
    }

    as_record_init(&record, properties->Length());
    for (uint32_t i = 0 ; i < properties->Length() ; ++i)
    {
      Handle<Value> rec_name = properties->Get(i);
      String::AsciiValue rec_str(rec_name);
      if (rec_str.length() == 0)
      {
        ThrowException(Exception::TypeError(String::New("one record property could not be converted into a valid ascii string")));
        as_record_destroy(&record);
        return false;
      }
      else if (rec_str.length() > AS_BIN_NAME_MAX_LEN)
      {
        ThrowException(Exception::TypeError(String::New("one record property is too big (15 characters max)")));
        as_record_destroy(&record);
        return false;
      }
      Handle<Value> value = rec->Get(rec_name);
      if (value->IsNumber())
      {
        as_record_set_int64(&record, *rec_str, value->ToNumber()->Value());
      }
      else if (value->IsString())
      {
        String::AsciiValue value_str(value);
        if (value_str.length() == 0)
        {
          as_record_set_raw(&record, *rec_str, reinterpret_cast<const uint8_t*>(*String::Utf8Value(value->ToString())), value->ToString()->Utf8Length());
        }
        else
        {
          as_record_set_str(&record, *rec_str, strdup(*value_str));
        }
      }
      else if (value->IsNull())
      {
        as_record_set_nil(&record, *rec_str);
      }
      else
      {
        ThrowException(Exception::TypeError(String::New("Invalid type for one value in the record.")));
        as_record_destroy(&record);
        return false;
      }
    }

    return true;
  }

  bool getRecordFromKeyRecordObject(const Handle<Object> &obj, as_record &record)
  {
    Handle<Value> rec = obj->Get(String::New("record"));

    if (!rec->IsObject())
    {
      ThrowException(Exception::TypeError(String::New("Expected object type for \"record\" property.")));
      return false;
    }

    return getRecordFromObject(Handle<Object>::Cast(rec), record);
  }

  bool getRecordFromKeyRecordArg(const Handle<Value> &arg, as_record &record)
  {
    if (!arg->IsObject())
    {
      ThrowException(Exception::TypeError(String::New("Expected object type for argument.")));
      return false;
    }

    return getRecordFromKeyRecordObject(Handle<Object>::Cast(arg), record);
  }

} // unamed namespace

Client::Client()
  : initialized(false)
  , connected(false)
{
  as_error_init(&err);
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

  Persistent<Function> constructor = Persistent<Function>::New(MY_NODE_ISOLATE_PRE tpl->GetFunction());
  target->Set(String::NewSymbol("Client"), constructor);
}

Handle<Value> Client::New(const Arguments& args)
{
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

  as_config_init(&client->config);

  // Read the param
  if (args.Length() != 2)
  {
    ThrowException(Exception::TypeError(String::New("Invalid number of arguments.")));
    return scope.Close(Undefined());
  }

  uint8_t hosts = 0;

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
      hosts++;
    }
  }
  else if (args[0]->IsObject())
  {
    if (!initConfigEntry(Handle<Object>::Cast(args[0]), 0, client->config))
      return scope.Close(Undefined());
    hosts++;
  }
  else
  {
    ThrowException(Exception::TypeError(String::New("Wrong first argument type (must be either object or array of objects)")));
    return scope.Close(Undefined());
  }

  if (hosts == 0)
  {
    ThrowException(Exception::TypeError(String::New("No valid host provided")));
    return scope.Close(Undefined());
  }

  // Run connect asynchronously
  BatonBase* baton = new BatonBase();
  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(cb);
  baton->client = client;

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonBase* baton = static_cast<BatonBase*>(req->data);
                  aerospike_init(&baton->client->as, &baton->client->config);
                  baton->status = aerospike_connect(&baton->client->as, &baton->error);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  HandleScope scope;
                  BatonBase* baton = static_cast<BatonBase*>(req->data);

                  baton->client->initialized = true;

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->status == AEROSPIKE_OK)
                  {
                    baton->client->connected = true;
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    // TODO: Create an error object
                    argv[0] = Integer::New(baton->status);
                  }
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  baton->callback.Dispose();
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

  // Run close asynchronously
  BatonBase* baton = new BatonBase();
  baton->request.data = baton;
  if (!cb.IsEmpty())
    baton->callback = Persistent<Function>::New(cb);
  baton->client = client;

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonBase* baton = static_cast<BatonBase*>(req->data);
                  baton->status = baton->client->close(baton->error);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  HandleScope scope;
                  BatonBase* baton = static_cast<BatonBase*>(req->data);

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->status == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    // TODO
                    argv[0] = Integer::New(baton->status);
                  }
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  baton->callback.Dispose();
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
    connected = false;
  }
  if (initialized)
  {
    aerospike_destroy(&as);
    initialized = false;
  }
  return result;
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
  Local<Function> cb = Local<Function>::Cast(args[1]);

  BatonKeyExists *baton = new BatonKeyExists();

  if (!getKeyFromArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(cb);
  baton->client = client;

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKeyExists* baton = static_cast<BatonKeyExists*>(req->data);
                  baton->status = aerospike_key_exists(&baton->client->as, &baton->client->err, NULL, &baton->key, &baton->result);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  HandleScope scope;
                  BatonKeyExists* baton = static_cast<BatonKeyExists*>(req->data);

                  const unsigned argc = 2;
                  Local<Value> argv[argc];
                  if (baton->status == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                    argv[1] = Local<Value>::New(baton->result ? True() : False());
                  }
                  else
                  {
                    // TODO: Create an error object
                    argv[0] = Integer::New(baton->status);
                    argv[1] = Local<Value>::New(Undefined());
                  }
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  baton->callback.Dispose();
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

  BatonKeyPut *baton = new BatonKeyPut();

  if (!getKeyFromKeyBinsArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  if (!getRecordFromKeyRecordArg(args[0], baton->record))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[2]));
  baton->client = client;

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKeyPut* baton = static_cast<BatonKeyPut*>(req->data);
                  baton->status = aerospike_key_put(&baton->client->as, &baton->client->err, NULL, &baton->key, &baton->record);
                  as_record_destroy(&baton->record);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  HandleScope scope;
                  BatonKeyPut* baton = static_cast<BatonKeyPut*>(req->data);

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->status == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    // TODO: Create an error object
                    argv[0] = Integer::New(baton->status);
                  }
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  baton->callback.Dispose();
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

  BatonKeyGet *baton = new BatonKeyGet();

  if (!getKeyFromKeyBinsArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
  baton->client = client;

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
                    baton->status = aerospike_key_get(&baton->client->as, &baton->error, NULL, &baton->key, &baton->record);
                  }
                  else
                  {
                    // Return only requested bins
                    baton->status = aerospike_key_select(&baton->client->as, &baton->error, NULL, &baton->key, const_cast<const char**>(baton->bins), &baton->record);
                  }
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  HandleScope scope;
                  BatonKeyGet* baton = static_cast<BatonKeyGet*>(req->data);

                  const unsigned argc = 2;
                  Local<Value> argv[argc];
                  if (baton->status == AEROSPIKE_OK)
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
                    // TODO: Create an error object
                    argv[0] = Integer::New(baton->status);
                    argv[1] = Local<Value>::New(Undefined());
                  }
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  baton->callback.Dispose();
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

  BatonKey *baton = new BatonKey();

  if (!getKeyFromArg(args[0], baton->key))
  {
    delete baton;
    return scope.Close(Undefined());
  }

  baton->request.data = baton;
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));
  baton->client = client;

  uv_queue_work(uv_default_loop(), &baton->request,
                /*AsyncWork*/
                [] (uv_work_t* req) {
                  BatonKey* baton = static_cast<BatonKey*>(req->data);
                  baton->status = aerospike_key_remove(&baton->client->as, &baton->client->err, NULL, &baton->key);
                },
                /*AsyncAfter*/
                [] (uv_work_s* req, int status) {
                  HandleScope scope;
                  BatonKey* baton = static_cast<BatonKey*>(req->data);

                  const unsigned argc = 1;
                  Local<Value> argv[argc];
                  if (baton->status == AEROSPIKE_OK)
                  {
                    argv[0] = Local<Value>::New(Undefined());
                  }
                  else
                  {
                    // TODO: Create an error object
                    argv[0] = Integer::New(baton->status);
                  }
                  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
                  baton->callback.Dispose();
                  delete baton;
                }
  );

  return scope.Close(Undefined());
}

} // namespace nodejs_aerospike
