

#ifndef CallData_h
#define CallData_h

#include "NativeFunctionWrapper.h"

namespace JSC {

    class ArgList;
    class ExecState;
    class FunctionBodyNode;
    class JSObject;
    class JSValue;
    class ScopeChainNode;

    enum CallType {
        CallTypeNone,
        CallTypeHost,
        CallTypeJS
    };

    typedef JSValue (JSC_HOST_CALL *NativeFunction)(ExecState*, JSObject*, JSValue thisValue, const ArgList&);

    union CallData {
        struct {
            NativeFunction function;
        } native;
        struct {
            FunctionBodyNode* functionBody;
            ScopeChainNode* scopeChain;
        } js;
    };

    JSValue call(ExecState*, JSValue functionObject, CallType, const CallData&, JSValue thisValue, const ArgList&);

} // namespace JSC

#endif // CallData_h
