

#ifndef ScriptValue_h
#define ScriptValue_h

#include "PlatformString.h"
#include "ScriptState.h"
#include <runtime/Protect.h>

namespace WebCore {

class String;

class ScriptValue {
public:
    ScriptValue(JSC::JSValue value = JSC::JSValue()) : m_value(value) {}
    virtual ~ScriptValue() {}

    JSC::JSValue jsValue() const { return m_value.get(); }
    bool getString(String& result) const;
    String toString(ScriptState* scriptState) const { return m_value.get().toString(scriptState); }
    bool isEqual(ScriptState*, const ScriptValue&) const;
    bool isNull() const;
    bool isUndefined() const;
    bool hasNoValue() const { return m_value == JSC::JSValue(); }

private:
    JSC::ProtectedJSValue m_value;
};

} // namespace WebCore

#endif // ScriptValue_h
