

#ifndef Debugger_h
#define Debugger_h

#include "Protect.h"

namespace JSC {

    class DebuggerCallFrame;
    class ExecState;
    class JSGlobalObject;
    class SourceCode;
    class UString;

    class Debugger {
    public:
        Debugger();
        virtual ~Debugger();

        void attach(JSGlobalObject*);
        virtual void detach(JSGlobalObject*);

        virtual void sourceParsed(ExecState*, const SourceCode&, int errorLine, const UString& errorMsg) = 0;
        virtual void exception(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;
        virtual void atStatement(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;
        virtual void callEvent(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;
        virtual void returnEvent(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;

        virtual void willExecuteProgram(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;
        virtual void didExecuteProgram(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;
        virtual void didReachBreakpoint(const DebuggerCallFrame&, intptr_t sourceID, int lineno) = 0;

    private:
        HashSet<JSGlobalObject*> m_globalObjects;
    };

    // This method exists only for backwards compatibility with existing
    // WebScriptDebugger clients
    JSValue evaluateInGlobalCallFrame(const UString&, JSValue& exception, JSGlobalObject*);

} // namespace JSC

#endif // Debugger_h
