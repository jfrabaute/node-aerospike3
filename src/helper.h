#ifndef SRC_HELPER_H
#define SRC_HELPER_H

extern "C" {
  #include <aerospike/aerospike.h>
}

#include <map>

namespace nodejs_aerospike {
namespace helper {

using namespace v8;

class Helper
{
public:

  static bool GetKeyFromObject(const Handle<Object> &arg, as_key &key);
  static bool GetKeyFromKeyBinsArg(const Handle<Value> &arg, as_key &key);
  static bool GetKeyFromArg(const Handle<Value> &arg, as_key &key);
  static bool GetRecordFromObject(const Handle<Object> &rec, as_record &record);
  static bool GetRecordFromKeyRecordObject(const Handle<Object> &obj, as_record &record);
  static bool GetRecordFromKeyRecordArg(const Handle<Value> &arg, as_record &record);

  static bool PopulateOps(const Handle<Object> &obj, DataKeyOperate *dataOps);

private:
  static bool OpsAppendStr(const Handle<Object> &obj, DataKeyOperate *dataOps);
  static bool OpsIncr(const Handle<Object> &obj, DataKeyOperate *dataOps);
  static bool OpsPrependStr(const Handle<Object> &obj, DataKeyOperate *dataOps);
  static bool OpsRead(const Handle<Object> &obj, DataKeyOperate *dataOps);
  static bool OpsTouch(const Handle<Object> &obj, DataKeyOperate *dataOps);
  static bool OpsWriteStr(const Handle<Object> &obj, DataKeyOperate *dataOps);
  static bool OpsWriteInt64(const Handle<Object> &obj, DataKeyOperate *dataOps);

  typedef bool (*OpFunction)(const Handle<Object> &obj, DataKeyOperate *dataOps);

  static std::map<std::string, OpFunction> OpsFactory;
};

} // namespace helper
} // namespace nodejs_aerospike

#endif // SRC_HELPER_H
