

#ifndef ConstructData_h
#define ConstructData_h

namespace JSC {

    class ArgList;
    class ExecState;
    class FunctionBodyNode;
    class JSObject;
    class JSValue;
    class ScopeChainNode;

    enum ConstructType {
        ConstructTypeNone,
        ConstructTypeHost,
        ConstructTypeJS
    };

    typedef JSObject* (*NativeConstructor)(ExecState*, JSObject*, const ArgList&);

    union ConstructData {
        struct {
            NativeConstructor function;
        } native;
        struct {
            FunctionBodyNode* functionBody;
            ScopeChainNode* scopeChain;
        } js;
    };

    JSObject* construct(ExecState*, JSValue constructor, ConstructType, const ConstructData&, const ArgList&);

} // namespace JSC

#endif // ConstructData_h
