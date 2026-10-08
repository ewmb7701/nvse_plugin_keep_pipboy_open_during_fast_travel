#pragma once
#include <cstdint>
#include <cstddef>
struct NVSEInterface {
 uint32_t nvseVersion,runtimeVersion,editorVersion,isEditor;
 void* RegisterCommand; void* SetOpcodeBase;
 void* (*QueryInterface)(uint32_t);
 uint32_t (*GetPluginHandle)();
 void* RegisterTypedCommand;
 const char* (*GetRuntimeDirectory)();
 uint32_t isNogore;
};
struct PluginInfo { uint32_t infoVersion; const char* name; uint32_t version; };
struct Messaging {
 struct Message { const char* sender; uint32_t type,dataLen; void* data; };
 uint32_t version;
 bool (*RegisterListener)(uint32_t,const char*,void(*)(Message*));
 void* Dispatch;
};
static_assert(sizeof(void*) == 4);
