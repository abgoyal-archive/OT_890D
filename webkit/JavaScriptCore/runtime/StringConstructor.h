

#ifndef StringConstructor_h
#define StringConstructor_h

#include "InternalFunction.h"

namespace JSC {

    class StringPrototype;

    class StringConstructor : public InternalFunction {
    public:
        StringConstructor(ExecState*, PassRefPtr<Structure>, Structure* prototypeFunctionStructure, StringPrototype*);

        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);
    };

} // namespace JSC

#endif // StringConstructor_h
