

#ifndef NumberPrototype_h
#define NumberPrototype_h

#include "NumberObject.h"

namespace JSC {

    class NumberPrototype : public NumberObject {
    public:
        NumberPrototype(ExecState*, PassRefPtr<Structure>, Structure* prototypeFunctionStructure);
    };

} // namespace JSC

#endif // NumberPrototype_h
