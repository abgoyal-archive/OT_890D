

#ifndef NumberConstructor_h
#define NumberConstructor_h

#include "InternalFunction.h"

namespace JSC {

    class NumberPrototype;

    class NumberConstructor : public InternalFunction {
    public:
        NumberConstructor(ExecState*, PassRefPtr<Structure>, NumberPrototype*);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
        JSValue getValueProperty(ExecState*, int token) const;

        static const ClassInfo info;

        static PassRefPtr<Structure> createStructure(JSValue proto) 
        { 
            return Structure::create(proto, TypeInfo(ObjectType, ImplementsHasInstance)); 
        }

        enum { NaNValue, NegInfinity, PosInfinity, MaxValue, MinValue };

    private:
        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);

        virtual const ClassInfo* classInfo() const { return &info; }
    };

} // namespace JSC

#endif // NumberConstructor_h
