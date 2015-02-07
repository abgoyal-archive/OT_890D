

#ifndef MathObject_h
#define MathObject_h

#include "JSObject.h"

namespace JSC {

    class MathObject : public JSObject {
    public:
        MathObject(ExecState*, PassRefPtr<Structure>);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;

        static PassRefPtr<Structure> createStructure(JSValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType));
        }
    };

} // namespace JSC

#endif // MathObject_h
