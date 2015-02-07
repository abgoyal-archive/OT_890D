

#ifndef ScriptState_h
#define ScriptState_h

#include "JSDOMBinding.h"

namespace WebCore {
    class Node;
    class Page;

    // The idea is to expose "state-like" methods (hadException, and any other
    // methods where ExecState just dips into globalData) of JSC::ExecState as a
    // separate abstraction.
    // For now, the separation is purely by convention.
    typedef JSC::ExecState ScriptState;

    ScriptState* scriptStateFromNode(Node*);
    ScriptState* scriptStateFromPage(Page*);

} // namespace WebCore

#endif // ScriptState_h
