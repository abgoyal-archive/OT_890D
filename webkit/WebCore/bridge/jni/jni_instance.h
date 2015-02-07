

#ifndef _JNI_INSTANCE_H_
#define _JNI_INSTANCE_H_

#if ENABLE(MAC_JAVA_BRIDGE)

#include "runtime.h"
#include "runtime_root.h"

#include <JavaVM/jni.h>

#if PLATFORM(ANDROID)
namespace android {
class WeakJavaInstance;
}
#endif

namespace JSC {

namespace Bindings {

class JavaClass;

class JObjectWrapper
{
friend class RefPtr<JObjectWrapper>;
friend class JavaArray;
friend class JavaField;
friend class JavaInstance;
friend class JavaMethod;
#if PLATFORM(ANDROID)
friend class android::WeakJavaInstance;
#endif

protected:
    JObjectWrapper(jobject instance);    
    ~JObjectWrapper();
    
    void ref() { _refCount++; }
    void deref() 
    { 
        if (--_refCount == 0) 
            delete this; 
    }

    jobject _instance;

private:
    JNIEnv *_env;
    unsigned int _refCount;
};

class JavaInstance : public Instance
{
public:
    static PassRefPtr<JavaInstance> create(jobject instance, PassRefPtr<RootObject> rootObject) 
    {
        return adoptRef(new JavaInstance(instance, rootObject));
    }
    
    ~JavaInstance();
    
    virtual Class *getClass() const;
    
    virtual JSValue valueOf(ExecState*) const;
    virtual JSValue defaultValue(ExecState*, PreferredPrimitiveType) const;

    virtual JSValue invokeMethod(ExecState* exec, const MethodList& method, const ArgList& args);

    jobject javaInstance() const { return _instance->_instance; }
    
    JSValue stringValue(ExecState*) const;
    JSValue numberValue(ExecState*) const;
    JSValue booleanValue() const;

protected:
    virtual void virtualBegin();
    virtual void virtualEnd();

#if !PLATFORM(ANDROID) // Submit patch to webkit.org
private:
#endif
    JavaInstance(jobject instance, PassRefPtr<RootObject>);

    RefPtr<JObjectWrapper> _instance;
    mutable JavaClass *_class;
};

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(MAC_JAVA_BRIDGE)

#endif // _JNI_INSTANCE_H_
