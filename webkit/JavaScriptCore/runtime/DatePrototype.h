

#ifndef DatePrototype_h
#define DatePrototype_h

#include "DateInstance.h"

namespace JSC {

    class ObjectPrototype;

    class DatePrototype : public DateInstance {
    public:
        DatePrototype(ExecState*, PassRefPtr<Structure>);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;

        static PassRefPtr<Structure> createStructure(JSValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType));
        }
    };

} // namespace JSC

#endif // DatePrototype_h
