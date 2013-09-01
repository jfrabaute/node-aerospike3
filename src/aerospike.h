#ifndef SRC_AEROSPIKE_H
#define SRC_AEROSPIKE_H

// NODE_MODULE_VERSION was incremented for v0.11
#if NODE_MODULE_VERSION > 0x000B
#  define MY_NODE_ISOLATE_DECL Isolate* isolate = Isolate::GetCurrent();
#  define MY_NODE_ISOLATE      isolate
#  define MY_NODE_ISOLATE_PRE  isolate, 
#  define MY_NODE_ISOLATE_POST , isolate 
#  define MY_HANDLESCOPE v8::HandleScope scope(MY_NODE_ISOLATE);
#else
#  define MY_NODE_ISOLATE_DECL
#  define MY_NODE_ISOLATE
#  define MY_NODE_ISOLATE_PRE
#  define MY_NODE_ISOLATE_POST
#  define MY_HANDLESCOPE v8::HandleScope scope;
#endif

#endif // SRC_AEROSPIKE_H
