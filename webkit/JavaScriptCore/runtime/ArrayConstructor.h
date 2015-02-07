

#ifndef ArrayConstructor_h
#define ArrayConstructor_h

#include "InternalFunction.h"

namespace JSC {

    class ArrayPrototype;

    class ArrayConstructor : public InternalFunction {
    public:
        ArrayConstructor(ExecState*, PassRefPtr<Structure>, ArrayPrototype*);

        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallData(CallData&);
    };

} // namespace JSC

#endif // ArrayConstructor_h
