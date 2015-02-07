

#ifndef StringPrototype_h
#define StringPrototype_h

#include "StringObject.h"

namespace JSC {

    class ObjectPrototype;

    class StringPrototype : public StringObject {
    public:
        StringPrototype(ExecState*, PassRefPtr<Structure>);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
    };

} // namespace JSC

#endif // StringPrototype_h
