

#ifndef NumberObject_h
#define NumberObject_h

#include "JSWrapperObject.h"

namespace JSC {

    class NumberObject : public JSWrapperObject {
    public:
        explicit NumberObject(PassRefPtr<Structure>);

        static const ClassInfo info;

    private:
        virtual const ClassInfo* classInfo() const { return &info; }

        virtual JSValue getJSNumber();
    };

    NumberObject* constructNumber(ExecState*, JSValue);

} // namespace JSC

#endif // NumberObject_h
