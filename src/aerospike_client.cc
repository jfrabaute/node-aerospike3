
#include <node.h>

#include "aerospike.h"
#include "aerospike_client.h"

using namespace v8;

namespace nodejs_aerospike {

namespace {

  bool initConfigEntry(const Handle<Object> &object, uint32_t id, as_config &config)
  {
    Handle<Value> addrJs = object->Get(String::New("addr"));
    Handle<Value> port = object->Get(String::New("port"));
    if (!addrJs->IsString())
    {
      ThrowException(Exception::TypeError(String::New("Missing or non string \"addr\" property in config object")));
      return false;
    }
    if (!port->IsUint32())
    {
      ThrowException(Exception::TypeError(String::New("Missing or non unsigned int \"port\" property in config object")));
      return false;
    }
  
    String::AsciiValue addr(addrJs);
    if (addr.length() == 0)
    {
      ThrowException(Exception::TypeError(String::New("\"addr\" property could not be converted into a valid ascii string")));
      return false;
    }

    config.hosts[id].addr = strdup(*addr);
    if (config.hosts[id].addr == NULL)
    {
      ThrowException(Exception::TypeError(String::New("Not Enough Memory: Unable to copy the addr data")));
      return false;
    }
    config.hosts[id].port = port->Uint32Value();

    return true;
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
  close();
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

  client->close();

  as_config_init(&client->config);

  // Read the param
  if (args.Length() < 1)
  {
    ThrowException(Exception::TypeError(String::New("Missing config argument (object or array of objects)")));
    return scope.Close(Undefined());
  }

  uint8_t hosts = 0;

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
    ThrowException(Exception::TypeError(String::New("Wrong argument type (must be either object or array of objects)")));
    return scope.Close(Undefined());
  }

  if (hosts == 0)
  {
    ThrowException(Exception::TypeError(String::New("No valid host provided")));
    return scope.Close(Undefined());
  }

  aerospike_init(&client->as, &client->config);
  client->initialized = true;
  as_status connect_status = aerospike_connect(&client->as, &client->err);
  if (connect_status != AEROSPIKE_OK)
  {
    if (args.Length() >= 2 && args[1]->IsFunction())
    {
      Local<Function> cb = Local<Function>::Cast(args[1]);
      const unsigned argc = 1;
      Local<Value> argv[argc] = { Local<Value>::New(String::Concat(String::New("Unable to connect to cluster. Error: "), Int32::New(connect_status)->ToString())) };
      cb->Call(Context::GetCurrent()->Global(), argc, argv);
      return scope.Close(Undefined());
    }
  }
  else
  {
    client->connected = true;
  }

  if (args.Length() >= 2 && args[1]->IsFunction())
  {
    Local<Function> cb = Local<Function>::Cast(args[1]);
    const unsigned argc = 1;
    Local<Value> argv[argc] = { Local<Value>::New(Undefined()) };
    cb->Call(Context::GetCurrent()->Global(), argc, argv);
  }

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

  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());
  client->close();

  return scope.Close(Undefined());
}

void Client::close()
{
  if (connected)
  {
    aerospike_close(&as, &err);
    connected = false;
  }
  if (initialized)
  {
    aerospike_destroy(&as);
    initialized = false;
  }
}


} // namespace nodejs_aerospike
