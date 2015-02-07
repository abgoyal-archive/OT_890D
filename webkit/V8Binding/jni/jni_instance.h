

#ifndef _JNI_INSTANCE_H_
#define _JNI_INSTANCE_H_

#include <JavaVM/jni.h>
#include <wtf/RefPtr.h>

namespace android {
class WeakJavaInstance;
}

using namespace WTF;

namespace JSC {

namespace Bindings {

class JavaClass;

class JObjectWrapper
{
public:
    JObjectWrapper(jobject instance);    
    ~JObjectWrapper();
    
    void ref() { _refCount++; }
    void deref() 
    { 
        if (--_refCount == 0) 
            delete this; 
    }

    jobject getLocalRef() const;

private:
    JNIEnv *_env;
    unsigned int _refCount;
    jobject _instance;  // it is a global weak reference.

    jmethodID mWeakRefGet;  // cache WeakReference::Get method id
};

class JavaInstance
{
public:
    JavaInstance(jobject instance);
    ~JavaInstance();

    void ref() { _refCount++; }
    void deref() 
    { 
        if (--_refCount == 0) 
            delete this; 
    }

    JavaClass* getClass() const;

    bool invokeMethod(const char* name, const NPVariant* args, uint32_t argsCount, NPVariant* result);

    // Returns a local reference, and the caller must delete
    // the returned reference after use.
    jobject getLocalRef() const {
        return _instance->getLocalRef();
    }

private:
    RefPtr<JObjectWrapper> _instance;
    unsigned int _refCount;
    mutable JavaClass* _class;
};

} // namespace Bindings

} // namespace JSC

#endif // _JNI_INSTANCE_H_
