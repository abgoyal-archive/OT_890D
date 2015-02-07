

#ifndef ScriptString_h
#define ScriptString_h

#include "PlatformString.h"
#include <runtime/UString.h>

namespace WebCore {

class String;

class ScriptString {
public:
    ScriptString() {}
    ScriptString(const char* s) : m_str(s) {}
    ScriptString(const JSC::UString& s) : m_str(s) {}

    operator JSC::UString() const { return m_str; }

    bool isNull() const { return m_str.isNull(); }
    size_t size() const { return m_str.size(); }

    ScriptString& operator=(const char* s)
    {
        m_str = s;
        return *this;
    }

    ScriptString& operator+=(const String& s)
    {
        m_str += s;
        return *this;
    }

    bool operator==(const ScriptString& s) const
    {
        return m_str == s.m_str;
    }

    bool operator!=(const ScriptString& s) const
    {
        // Avoid exporting an extra symbol by re-using "==" operator.
        return !(m_str == s.m_str);
    }

private:
    JSC::UString m_str;
};

} // namespace WebCore

#endif // ScriptString_h
