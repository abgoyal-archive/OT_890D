

#ifndef SQLValue_h
#define SQLValue_h

#include "PlatformString.h"
#include <wtf/Threading.h>

namespace WebCore {

    class SQLValue {
    public:
        enum Type { NullValue, NumberValue, StringValue };

        SQLValue() : m_type(NullValue) { }
        SQLValue(double number) : m_type(NumberValue), m_number(number) { }
        SQLValue(const String& s) : m_type(StringValue), m_string(s) { }
        SQLValue(const SQLValue&);

        Type type() const { return m_type; }

        String string() const;
        double number() const;

    private:
        Type m_type;
        double m_number;
        String m_string;
    };
};

#endif
