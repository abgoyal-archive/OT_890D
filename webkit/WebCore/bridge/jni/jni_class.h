

#ifndef JNI_CLASS_H_
#define JNI_CLASS_H_

#if ENABLE(MAC_JAVA_BRIDGE)

#include <jni_runtime.h>
#include <wtf/HashMap.h>

namespace JSC {

namespace Bindings {

class JavaClass : public Class {
public:
    JavaClass (jobject anInstance);
    ~JavaClass ();

    virtual MethodList methodsNamed(const Identifier&, Instance* instance) const;    
    virtual Field *fieldNamed(const Identifier&, Instance* instance) const;
    
    bool isNumberClass() const;
    bool isBooleanClass() const;
    bool isStringClass() const;
    
private:
    const char *_name;
    FieldMap _fields;
    MethodListMap _methods;
};

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(MAC_JAVA_BRIDGE)

#endif // JNI_CLASS_H_
