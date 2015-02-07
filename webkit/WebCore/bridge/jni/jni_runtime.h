

#ifndef _JNI_RUNTIME_H_
#define _JNI_RUNTIME_H_

#if ENABLE(MAC_JAVA_BRIDGE)

#include <jni_utility.h>
#include <jni_instance.h>
#include <runtime/JSLock.h>


namespace JSC
{

namespace Bindings
{

typedef const char* RuntimeType;

class JavaString
{
public:
    JavaString()
    {
        JSLock lock(SilenceAssertionsOnly);
        _rep = UString().rep();
    }

    void _commonInit (JNIEnv *e, jstring s)
    {
        int _size = e->GetStringLength (s);
        const jchar *uc = getUCharactersFromJStringInEnv (e, s);
        {
            JSLock lock(SilenceAssertionsOnly);
            _rep = UString(reinterpret_cast<const UChar*>(uc), _size).rep();
        }
        releaseUCharactersForJStringInEnv (e, s, uc);
    }
    
    JavaString (JNIEnv *e, jstring s) {
        _commonInit (e, s);
    }
    
    JavaString (jstring s) {
        _commonInit (getJNIEnv(), s);
    }
    
    ~JavaString()
    {
        JSLock lock(SilenceAssertionsOnly);
        _rep = 0;
    }
    
    const char *UTF8String() const { 
        if (_utf8String.c_str() == 0) {
            JSLock lock(SilenceAssertionsOnly);
            _utf8String = UString(_rep).UTF8String();
        }
        return _utf8String.c_str();
    }
    const jchar *uchars() const { return (const jchar *)_rep->data(); }
    int length() const { return _rep->size(); }
    operator UString() const { return UString(_rep); }

private:
    RefPtr<UString::Rep> _rep;
    mutable CString _utf8String;
};

class JavaParameter
{
public:
    JavaParameter () : _JNIType(invalid_type) {};
    JavaParameter (JNIEnv *env, jstring type);
    virtual ~JavaParameter() { }

    RuntimeType type() const { return _type.UTF8String(); }
    JNIType getJNIType() const { return _JNIType; }
    
private:
    JavaString _type;
    JNIType _JNIType;
};


class JavaField : public Field
{
public:
    JavaField (JNIEnv *env, jobject aField);

    virtual JSValue valueFromInstance(ExecState *exec, const Instance *instance) const;
    virtual void setValueToInstance(ExecState *exec, const Instance *instance, JSValue aValue) const;
    
    UString::Rep* name() const { return ((UString)_name).rep(); }
    virtual RuntimeType type() const { return _type.UTF8String(); }

    JNIType getJNIType() const { return _JNIType; }
    
private:
    void dispatchSetValueToInstance(ExecState *exec, const JavaInstance *instance, jvalue javaValue, const char *name, const char *sig) const;
    jvalue dispatchValueFromInstance(ExecState *exec, const JavaInstance *instance, const char *name, const char *sig, JNIType returnType) const;

    JavaString _name;
    JavaString _type;
    JNIType _JNIType;
    RefPtr<JObjectWrapper> _field;
};


class JavaMethod : public Method
{
public:
    JavaMethod(JNIEnv* env, jobject aMethod);
    ~JavaMethod();

    UString::Rep* name() const { return ((UString)_name).rep(); }
    RuntimeType returnType() const { return _returnType.UTF8String(); };
    JavaParameter* parameterAt(int i) const { return &_parameters[i]; };
    int numParameters() const { return _numParameters; };
    
    const char *signature() const;
    JNIType JNIReturnType() const;

    jmethodID methodID (jobject obj) const;
    
    bool isStatic() const { return _isStatic; }

private:
    JavaParameter* _parameters;
    int _numParameters;
    JavaString _name;
    mutable char* _signature;
    JavaString _returnType;
    JNIType _JNIReturnType;
    mutable jmethodID _methodID;
    bool _isStatic;
};

class JavaArray : public Array
{
public:
    JavaArray(jobject array, const char* type, PassRefPtr<RootObject>);
    virtual ~JavaArray();

    RootObject* rootObject() const;

    virtual void setValueAt(ExecState *exec, unsigned int index, JSValue aValue) const;
    virtual JSValue valueAt(ExecState *exec, unsigned int index) const;
    virtual unsigned int getLength() const;
    
    jobject javaArray() const { return _array->_instance; }

    static JSValue convertJObjectToArray (ExecState* exec, jobject anObject, const char* type, PassRefPtr<RootObject>);

private:
    RefPtr<JObjectWrapper> _array;
    unsigned int _length;
    const char *_type;
};

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(MAC_JAVA_BRIDGE)

#endif // _JNI_RUNTIME_H_
