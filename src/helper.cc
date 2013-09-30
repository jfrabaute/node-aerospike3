
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

namespace nodejs_aerospike {
namespace helper {

std::map<std::string, Helper::OpFunction> Helper::OpsFactory = {
  {"append_str",  &Helper::OpsAppendStr},
  {"incr",        &Helper::OpsIncr},
  {"prepend_str", &Helper::OpsPrependStr},
  {"read",        &Helper::OpsRead},
  {"touch",       &Helper::OpsTouch},
  {"write_str",   &Helper::OpsWriteStr},
  {"write_int64", &Helper::OpsWriteInt64}
};

bool Helper::PopulateOps(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  if (!GetKeyFromArg(obj->Get(String::NewSymbol("key")), dataOps->key))
    return false;

  Local<Value> opsObj = obj->Get(String::NewSymbol("ops"));
  if (!opsObj->IsArray())
  {
    ThrowException(Exception::TypeError(String::New("\"ops\" property not present or not an array")));
    return false;
  }

  // Populate ops
  Local<Array> ops = Local<Array>::Cast(opsObj);
  uint32_t len = ops->Length();
  if (len == 0)
  {
    ThrowException(Exception::TypeError(String::New("\"ops\" array is empty")));
    return false;
  }

  as_operations_init(&dataOps->ops, len);
  dataOps->destroy = true;

  Local<Value> entry;
  Local<Object> operation;

  Local<Value> op;
  for (uint32_t i = 0 ; i < len ; ++i)
  {
    entry = ops->Get(i);
    if (!entry->IsObject())
    {
      ThrowException(Exception::TypeError(String::New("One \"ops\" entry is not an object")));
      return false;
    }
    operation = Local<Object>::Cast(entry);
    op = operation->Get(String::NewSymbol("op"));
    if (!op->IsString())
    {
      ThrowException(Exception::TypeError(String::New("One \"op\" entry is not a string")));
      return false;
    }
    String::AsciiValue opname(op);
    if (opname.length() == 0)
    {
      ThrowException(Exception::TypeError(String::New("One \"op\" entry cannot be converted to an ascii string")));
      return false;
    }

    auto it = OpsFactory.find(*opname);
    if (it == OpsFactory.end())
    {
      ThrowException(Exception::TypeError(String::New("One operation is unknown or not implemented.")));
      return false;
    }

    if (it->second(operation, dataOps) == false)
      return false;
  }

  return true;
}

bool Helper::GetKeyFromObject(const Handle<Object> &arg, as_key &key)
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

bool Helper::GetKeyFromKeyBinsArg(const Handle<Value> &arg, as_key &key)
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

  return GetKeyFromObject(Handle<Object>::Cast(keyArg), key);
}

bool Helper::GetKeyFromArg(const Handle<Value> &arg, as_key &key)
{
  if (!arg->IsObject())
  {
    ThrowException(Exception::TypeError(String::New("Expected object type for key argument.")));
    return false;
  }

  return GetKeyFromObject(Handle<Object>::Cast(arg), key);
}

bool Helper::GetRecordFromObject(const Handle<Object> &rec, as_record &record)
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

bool Helper::GetRecordFromKeyRecordObject(const Handle<Object> &obj, as_record &record)
{
  Handle<Value> rec = obj->Get(String::New("record"));

  if (!rec->IsObject())
  {
    ThrowException(Exception::TypeError(String::New("Expected object type for \"record\" property.")));
    return false;
  }

  return GetRecordFromObject(Handle<Object>::Cast(rec), record);
}

bool Helper::GetRecordFromKeyRecordArg(const Handle<Value> &arg, as_record &record)
{
  if (!arg->IsObject())
  {
    ThrowException(Exception::TypeError(String::New("Expected object type for argument.")));
    return false;
  }

  return GetRecordFromKeyRecordObject(Handle<Object>::Cast(arg), record);
}

bool Helper::OpsAppendStr(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  Handle<Value> colVal = obj->Get(String::NewSymbol("col"));
  if (!colVal->IsString())
  {
    ThrowException(Exception::TypeError(String::New("\"col\" property is undefined or not a string")));
    return false;
  }
  String::AsciiValue col(colVal);
  if (col.length() == 0)
  {
    ThrowException(Exception::TypeError(String::New("Unable to convert \"col\" property value to ascii string")));
    return false;
  }
  else if (col.length() > AS_BIN_NAME_MAX_LEN)
  {
    ThrowException(Exception::TypeError(String::New("one \"col\" property is too big (15 characters max)")));
    return false;
  }
  dataOps->strings.push_back(*col);

  Handle<Value> valueVal = obj->Get(String::NewSymbol("value"));
  if (!valueVal->IsString())
  {
    ThrowException(Exception::TypeError(String::New("\"value\" property is undefined or not a string")));
    return false;
  }
  String::AsciiValue value(valueVal);
  if (value.length() == 0 && valueVal->ToString()->Length() != 0)
  {
    ThrowException(Exception::TypeError(String::New("Unable to convert \"value\" value to ascii string")));
    return false;
  }
  dataOps->strings.push_back(*value);

  if (!as_operations_add_append_str(&dataOps->ops,
                                    (dataOps->strings.rbegin()+1)->c_str(),
                                    dataOps->strings.back().c_str()))
  {
    ThrowException(Exception::TypeError(String::New("Unknown error in internal operation (as_operations_add_append_str)")));
    return false;
  }

  return true;
}

bool Helper::OpsIncr(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  Handle<Value> colVal = obj->Get(String::NewSymbol("col"));
  if (!colVal->IsString())
  {
    ThrowException(Exception::TypeError(String::New("\"col\" property is undefined or not a string")));
    return false;
  }
  String::AsciiValue col(colVal);
  if (col.length() == 0)
  {
    ThrowException(Exception::TypeError(String::New("Unable to convert \"col\" property value to ascii string")));
    return false;
  }
  else if (col.length() > AS_BIN_NAME_MAX_LEN)
  {
    ThrowException(Exception::TypeError(String::New("one \"col\" property is too big (15 characters max)")));
    return false;
  }
  dataOps->strings.push_back(*col);

  Handle<Value> valueVal = obj->Get(String::NewSymbol("value"));
  if (!valueVal->IsInt32())
  {
    ThrowException(Exception::TypeError(String::New("\"value\" property is undefined or not an int")));
    return false;
  }

  if (!as_operations_add_incr(&dataOps->ops,
                              dataOps->strings.back().c_str(),
                              valueVal->Int32Value()))
  {
    ThrowException(Exception::TypeError(String::New("Unknown error in internal operation (as_operations_add_incr)")));
    return false;
  }

  return true;
}

bool Helper::OpsPrependStr(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  ThrowException(Exception::TypeError(String::New("Operation not implemented yet")));
  return false;
}

bool Helper::OpsRead(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  Handle<Value> colVal = obj->Get(String::NewSymbol("col"));
  if (!colVal->IsString())
  {
    ThrowException(Exception::TypeError(String::New("\"col\" property is undefined or not a string")));
    return false;
  }
  String::AsciiValue col(colVal);
  if (col.length() == 0)
  {
    ThrowException(Exception::TypeError(String::New("Unable to convert \"col\" property value to ascii string")));
    return false;
  }
  else if (col.length() > AS_BIN_NAME_MAX_LEN)
  {
    ThrowException(Exception::TypeError(String::New("one \"col\" property is too big (15 characters max)")));
    return false;
  }
  dataOps->strings.push_back(*col);

  if (!as_operations_add_read(&dataOps->ops,
                              dataOps->strings.back().c_str()))
  {
    ThrowException(Exception::TypeError(String::New("Unknown error in internal operation (as_operations_add_read)")));
    return false;
  }

  return true;
}

bool Helper::OpsTouch(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  if (!as_operations_add_touch(&dataOps->ops))
  {
    ThrowException(Exception::TypeError(String::New("Unknown error in internal operation (as_operations_add_touch)")));
    return false;
  }

  return true;
}

bool Helper::OpsWriteStr(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  ThrowException(Exception::TypeError(String::New("Operation not implemented yet")));
  return false;
}

bool Helper::OpsWriteInt64(const Handle<Object> &obj, DataKeyOperate *dataOps)
{
  ThrowException(Exception::TypeError(String::New("Operation not implemented yet")));
  return false;
}

} // namespace helper
} // namespace nodejs_aerospike
