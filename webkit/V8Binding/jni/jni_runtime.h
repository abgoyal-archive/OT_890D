

#ifndef _JNI_RUNTIME_H_
#define _JNI_RUNTIME_H_

#include "jni_utility.h"
#include "jni_instance.h"

#include "CString.h"

namespace JSC
{

namespace Bindings
{

class JavaString
{
public:
    JavaString() { }

    void _commonInit (JNIEnv *e, jstring s)
    {
        int size = e->GetStringLength(s);
        const char* cs = getCharactersFromJStringInEnv(e, s);
        {
            _utf8String = WebCore::CString(cs, size);
        }
        releaseCharactersForJStringInEnv (e, s, cs);
    }
    
    JavaString (JNIEnv *e, jstring s) {
        _commonInit (e, s);
    }
    
    JavaString (jstring s) {
        _commonInit (getJNIEnv(), s);
    }
    
    ~JavaString() { }
    
    int length() const { return _utf8String.length(); }
    
    const char* UTF8String() const {
        return _utf8String.data();
    }

private:
    WebCore::CString _utf8String;
};


class JavaParameter
{
public:
    JavaParameter () : _JNIType(invalid_type) {};
    JavaParameter (JNIEnv *env, jstring type);
    virtual ~JavaParameter() { }

    const char* type() const { return _type.UTF8String(); }
    JNIType getJNIType() const { return _JNIType; }
    
private:
    JavaString _type;
    JNIType _JNIType;
};


class JavaField
{
public:
    JavaField (JNIEnv *env, jobject aField);

    const char* name() const { return _name.UTF8String(); }
    const char* type() const { return _type.UTF8String(); }

    JNIType getJNIType() const { return _JNIType; }
    
private:
    JavaString _name;
    JavaString _type;
    JNIType _JNIType;
    RefPtr<JObjectWrapper> _field;
};


class JavaMethod
{
public:
    JavaMethod(JNIEnv* env, jobject aMethod);
    ~JavaMethod();

    const char* name() const { return _name.UTF8String(); }
    const char* returnType() const { return _returnType.UTF8String(); };
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

} // namespace Bindings

} // namespace JSC

#endif // _JNI_RUNTIME_H_
