

#ifndef ObjectPrototype_h
#define ObjectPrototype_h

#include "JSObject.h"

namespace JSC {

    class ObjectPrototype : public JSObject {
    public:
        ObjectPrototype(ExecState*, PassRefPtr<Structure>, Structure* prototypeFunctionStructure);
    };

    JSValue JSC_HOST_CALL objectProtoFuncToString(ExecState*, JSObject*, JSValue, const ArgList&);

} // namespace JSC

#endif // ObjectPrototype_h
