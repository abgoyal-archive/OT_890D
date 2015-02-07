

#ifndef JNI_CLASS_H_
#define JNI_CLASS_H_

#include <jni_runtime.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include "PlatformString.h"
#include "StringHash.h"

namespace JSC {

namespace Bindings {

typedef Vector<JavaMethod*> MethodList;
typedef HashMap<WebCore::String, MethodList*> MethodListMap;
typedef HashMap<WebCore::String, JavaField*> FieldMap;

class JavaClass {
public:
    JavaClass (jobject anInstance);
    ~JavaClass ();

    MethodList methodsNamed(const char* name) const;    
    JavaField* fieldNamed(const char* name) const;

private:
    const char *_name;
    MethodListMap _methods;
    FieldMap _fields;
};

} // namespace Bindings

} // namespace JSC

#endif // JNI_CLASS_H_
