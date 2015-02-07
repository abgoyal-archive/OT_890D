

#ifndef V8Binding_h
#define V8Binding_h

#include "MathExtras.h"
#include "PlatformString.h"

#include <v8.h>

namespace WebCore {

    enum ExternalMode {
        Externalize,
        DoNotExternalize
    };

    enum StringType {
        PlainStringType,
        AtomicStringType
    };

    // Convert v8 types to a WebCore::String. If the V8 string is not already
    // an external string then it is transformed into an external string at this
    // point to avoid repeated conversions.
    String v8StringToWebCoreString(v8::Handle<v8::String>, ExternalMode mode, StringType type);
    String v8ValueToWebCoreString(v8::Handle<v8::Value>);

    // Convert v8 types to a WebCore::AtomicString.
    AtomicString v8StringToAtomicWebCoreString(v8::Handle<v8::String>);
    AtomicString v8ValueToAtomicWebCoreString(v8::Handle<v8::Value>);

    // Convert a string to a V8 string.
    v8::Handle<v8::String> v8String(const String&);

    inline String toString(const String& string)
    {
        return string;
    }

    // Return a V8 external string that shares the underlying buffer with the given
    // WebCore string. The reference counting mechanism is used to keep the
    // underlying buffer alive while the string is still live in the V8 engine.
    v8::Local<v8::String> v8ExternalString(const String&);

    // Enables caching v8 wrappers created for WebCore::StringImpl.  Currently this cache requires
    // all the calls (both to convert WebCore::String to v8::String and to GC the handle)
    // to be performed on the main thread.
    void enableStringImplCache();

    // Convert a value to a 32-bit integer.  The conversion fails if the
    // value cannot be converted to an integer or converts to nan or to an infinity.
    inline int toInt32(v8::Handle<v8::Value> value, bool& ok)
    {
        ok = true;

        // Fast case.  The value is already a 32-bit integer.
        if (value->IsInt32())
            return value->Int32Value();

        // Can the value be converted to a number?
        v8::Local<v8::Number> numberObject = value->ToNumber();
        if (numberObject.IsEmpty()) {
            ok = false;
            return 0;
        }

        // Does the value convert to nan or to an infinity?
        double numberValue = numberObject->Value();
        if (isnan(numberValue) || isinf(numberValue)) {
            ok = false;
            return 0;
        }

        // Can the value be converted to a 32-bit integer?
        v8::Local<v8::Int32> intValue = value->ToInt32();
        if (intValue.IsEmpty()) {
            ok = false;
            return 0;
        }

        // Return the result of the int32 conversion.
        return intValue->Value();
    }

    // Convert a value to a 32-bit integer assuming the conversion cannot fail.
    inline int toInt32(v8::Handle<v8::Value> value)
    {
        bool ok;
        return toInt32(value, ok);
    }

    inline float toFloat(v8::Local<v8::Value> value)
    {
        return static_cast<float>(value->NumberValue());
    }

    // FIXME: Drop this in favor of the type specific v8ValueToWebCoreString when we rework the code generation.
    inline String toWebCoreString(v8::Handle<v8::Value> object)
    {
        return v8ValueToWebCoreString(object);
    }

    // The string returned by this function is still owned by the argument
    // and will be deallocated when the argument is deallocated.
    inline const uint16_t* fromWebCoreString(const String& str)
    {
        return reinterpret_cast<const uint16_t*>(str.characters());
    }

    inline bool isUndefinedOrNull(v8::Handle<v8::Value> value)
    {
        return value->IsNull() || value->IsUndefined();
    }

    inline v8::Handle<v8::Boolean> v8Boolean(bool value)
    {
        return value ? v8::True() : v8::False();
    }
   
    inline String toWebCoreStringWithNullCheck(v8::Handle<v8::Value> value)
    {
        if (value->IsNull()) 
            return String();
        return v8ValueToWebCoreString(value);
    }

    inline String toWebCoreStringWithNullOrUndefinedCheck(v8::Handle<v8::Value> value)
    {
        if (value->IsNull() || value->IsUndefined())
            return String();
        return toWebCoreString(value);
    }
 
    inline v8::Handle<v8::String> v8UndetectableString(const String& str)
    {
        return v8::String::NewUndetectable(fromWebCoreString(str), str.length());
    }

    inline v8::Handle<v8::Value> v8StringOrNull(const String& str)
    {
        return str.isNull() ? v8::Handle<v8::Value>(v8::Null()) : v8::Handle<v8::Value>(v8String(str));
    }

    inline v8::Handle<v8::Value> v8StringOrUndefined(const String& str)
    {
        return str.isNull() ? v8::Handle<v8::Value>(v8::Undefined()) : v8::Handle<v8::Value>(v8String(str));
    }

    inline v8::Handle<v8::Value> v8StringOrFalse(const String& str)
    {
        return str.isNull() ? v8::Handle<v8::Value>(v8::False()) : v8::Handle<v8::Value>(v8String(str));
    }
} // namespace WebCore

#endif // V8Binding_h
