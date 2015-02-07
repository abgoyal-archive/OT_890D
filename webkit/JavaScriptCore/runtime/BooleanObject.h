

#ifndef BooleanObject_h
#define BooleanObject_h

#include "JSWrapperObject.h"

namespace JSC {

    class BooleanObject : public JSWrapperObject {
    public:
        explicit BooleanObject(PassRefPtr<Structure>);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
    };

    BooleanObject* asBooleanObject(JSValue);

    inline BooleanObject* asBooleanObject(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&BooleanObject::info));
        return static_cast<BooleanObject*>(asObject(value));
    }

} // namespace JSC

#endif // BooleanObject_h
