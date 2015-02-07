
#ifndef NPV8Object_h
#define NPV8Object_h

#if PLATFORM(CHROMIUM)
// TODO(andreip): diff and consolidate
#include "bindings/npruntime.h"
#else
#include "bridge/npruntime.h"  // use WebCore version
#endif
#include <v8.h>

namespace WebCore {
    class DOMWindow;
}

extern NPClass* npScriptObjectClass;

// A V8NPObject is a NPObject which carries additional V8-specific information. It is allocated and deallocated by
// AllocV8NPObject() and FreeV8NPObject() methods.
struct V8NPObject {
    NPObject object;
    v8::Persistent<v8::Object> v8Object;
    WebCore::DOMWindow* rootObject;
};

struct PrivateIdentifier {
    union {
        const NPUTF8* string;
        int32_t number;
    } value;
    bool isString;
};

NPObject* npCreateV8ScriptObject(NPP, v8::Handle<v8::Object>, WebCore::DOMWindow*);

#endif // NPV8Object_h
