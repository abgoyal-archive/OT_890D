

#ifndef ScheduledAction_h
#define ScheduledAction_h

#include "PlatformString.h"
#include <runtime/Protect.h>
#include <wtf/Vector.h>

namespace JSC {
    class JSGlobalObject;
}

namespace WebCore {

    class Document;
    class ScriptExecutionContext;
    class WorkerContext;

   /* An action (either function or string) to be executed after a specified
    * time interval, either once or repeatedly. Used for window.setTimeout()
    * and window.setInterval()
    */
    class ScheduledAction {
    public:
        static ScheduledAction* create(JSC::ExecState*, const JSC::ArgList&);

        void execute(ScriptExecutionContext*);

    private:
        ScheduledAction(JSC::JSValue function, const JSC::ArgList&);
        ScheduledAction(const String& code)
            : m_code(code)
        {
        }

        void executeFunctionInContext(JSC::JSGlobalObject*, JSC::JSValue thisValue);
        void execute(Document*);
#if ENABLE(WORKERS)        
        void execute(WorkerContext*);
#endif

        JSC::ProtectedJSValue m_function;
        Vector<JSC::ProtectedJSValue> m_args;
        String m_code;
    };

} // namespace WebCore

#endif // ScheduledAction_h
