

#ifndef ScriptString_h
#define ScriptString_h

#include "PlatformString.h"

namespace WebCore {

class ScriptString {
public:
    ScriptString() {}
    ScriptString(const String& s) : m_str(s) {}
    ScriptString(const char* s) : m_str(s) {}

    operator String() const { return m_str; }

    bool isNull() const { return m_str.isNull(); }
    size_t size() const { return m_str.length(); }

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

private:
    String m_str;
};

} // namespace WebCore

#endif // ScriptString_h
