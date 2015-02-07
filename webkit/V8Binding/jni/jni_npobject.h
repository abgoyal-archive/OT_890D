

#ifndef JNI_NPOBJECT_H_
#define JNI_NPOBJECT_H_

#include "npruntime.h"
#include "jni_runtime.h"

#include <wtf/RefPtr.h>
#include <JavaVM/jni.h>

namespace JSC { namespace Bindings {

struct JavaNPObject {
    NPObject _object;
    RefPtr<JavaInstance> _instance;
};

NPObject* JavaInstanceToNPObject(JavaInstance* instance);
JavaInstance* ExtractJavaInstance(NPObject* obj);

bool JavaNPObject_HasMethod(NPObject* obj, NPIdentifier name);
bool JavaNPObject_Invoke(NPObject* obj, NPIdentifier methodName, const NPVariant* args, uint32_t argCount, NPVariant* result);
bool JavaNPObject_HasProperty(NPObject* obj, NPIdentifier name);
bool JavaNPObject_GetProperty(NPObject* obj, NPIdentifier name, NPVariant* ressult);

} }

#endif JNI_NPOBJECT_H_
