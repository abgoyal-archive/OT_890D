

#ifndef InternalFunction_h
#define InternalFunction_h

#include "JSObject.h"
#include "Identifier.h"

namespace JSC {

    class FunctionPrototype;

    class InternalFunction : public JSObject {
    public:
        virtual const ClassInfo* classInfo() const; 
        static JS_EXPORTDATA const ClassInfo info;

        const UString& name(JSGlobalData*);
        const UString displayName(JSGlobalData*);
        const UString calculatedDisplayName(JSGlobalData*);

        static PassRefPtr<Structure> createStructure(JSValue proto) 
        { 
            return Structure::create(proto, TypeInfo(ObjectType, ImplementsHasInstance | HasStandardGetOwnPropertySlot)); 
        }

    protected:
        InternalFunction(PassRefPtr<Structure> structure) : JSObject(structure) { }
        InternalFunction(JSGlobalData*, PassRefPtr<Structure>, const Identifier&);

    private:
        virtual CallType getCallData(CallData&) = 0;
    };

    InternalFunction* asInternalFunction(JSValue);

    inline InternalFunction* asInternalFunction(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&InternalFunction::info));
        return static_cast<InternalFunction*>(asObject(value));
    }

} // namespace JSC

#endif // InternalFunction_h
