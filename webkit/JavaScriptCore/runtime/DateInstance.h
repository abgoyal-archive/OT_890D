

#ifndef DateInstance_h
#define DateInstance_h

#include "JSWrapperObject.h"

namespace WTF {
    struct GregorianDateTime;
}

namespace JSC {

    class DateInstance : public JSWrapperObject {
    public:
        explicit DateInstance(PassRefPtr<Structure>);
        virtual ~DateInstance();

        double internalNumber() const { return internalValue().uncheckedGetNumber(); }

        bool getTime(WTF::GregorianDateTime&, int& offset) const;
        bool getUTCTime(WTF::GregorianDateTime&) const;
        bool getTime(double& milliseconds, int& offset) const;
        bool getUTCTime(double& milliseconds) const;

        static const ClassInfo info;

        void msToGregorianDateTime(double, bool outputIsUTC, WTF::GregorianDateTime&) const;

    private:
        virtual const ClassInfo* classInfo() const { return &info; }

        using JSWrapperObject::internalValue;

        struct Cache;
        mutable Cache* m_cache;
    };

    DateInstance* asDateInstance(JSValue);

    inline DateInstance* asDateInstance(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&DateInstance::info));
        return static_cast<DateInstance*>(asObject(value));
    }

} // namespace JSC

#endif // DateInstance_h
