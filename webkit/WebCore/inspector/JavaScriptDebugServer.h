

#ifndef JavaScriptDebugServer_h
#define JavaScriptDebugServer_h

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include "Timer.h"
#include <debugger/Debugger.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

namespace JSC {
    class DebuggerCallFrame;
    class JSGlobalObject;
}

namespace WebCore {

    class Frame;
    class FrameView;
    class Page;
    class PageGroup;
    class JavaScriptCallFrame;
    class JavaScriptDebugListener;

    class JavaScriptDebugServer : JSC::Debugger {
    public:
        static JavaScriptDebugServer& shared();

        void addListener(JavaScriptDebugListener*);
        void removeListener(JavaScriptDebugListener*);

        void addListener(JavaScriptDebugListener*, Page*);
        void removeListener(JavaScriptDebugListener*, Page*);

        void addBreakpoint(intptr_t sourceID, unsigned lineNumber);
        void removeBreakpoint(intptr_t sourceID, unsigned lineNumber);
        bool hasBreakpoint(intptr_t sourceID, unsigned lineNumber) const;
        void clearBreakpoints();

        bool pauseOnExceptions() const { return m_pauseOnExceptions; }
        void setPauseOnExceptions(bool);

        void pauseProgram();
        void continueProgram();
        void stepIntoStatement();
        void stepOverStatement();
        void stepOutOfFunction();

        void recompileAllJSFunctionsSoon();
        void recompileAllJSFunctions(Timer<JavaScriptDebugServer>* = 0);

        JavaScriptCallFrame* currentCallFrame();

        void pageCreated(Page*);

        typedef HashSet<JavaScriptDebugListener*> ListenerSet;
        typedef void (JavaScriptDebugListener::*JavaScriptExecutionCallback)();

    private:
        JavaScriptDebugServer();
        ~JavaScriptDebugServer();

        bool hasListeners() const { return !m_listeners.isEmpty() || !m_pageListenersMap.isEmpty(); }
        bool hasGlobalListeners() const { return !m_listeners.isEmpty(); }
        bool hasListenersInterestedInPage(Page*);

        void setJavaScriptPaused(const PageGroup&, bool paused);
        void setJavaScriptPaused(Page*, bool paused);
        void setJavaScriptPaused(Frame*, bool paused);
        void setJavaScriptPaused(FrameView*, bool paused);

        void dispatchFunctionToListeners(JavaScriptExecutionCallback, Page*);
        void pauseIfNeeded(Page*);
        
        virtual void detach(JSC::JSGlobalObject*);

        virtual void sourceParsed(JSC::ExecState*, const JSC::SourceCode&, int errorLine, const JSC::UString& errorMsg);
        virtual void callEvent(const JSC::DebuggerCallFrame&, intptr_t sourceID, int lineNumber);
        virtual void atStatement(const JSC::DebuggerCallFrame&, intptr_t sourceID, int firstLine);
        virtual void returnEvent(const JSC::DebuggerCallFrame&, intptr_t sourceID, int lineNumber);
        virtual void exception(const JSC::DebuggerCallFrame&, intptr_t sourceID, int lineNumber);
        virtual void willExecuteProgram(const JSC::DebuggerCallFrame&, intptr_t sourceID, int lineno);
        virtual void didExecuteProgram(const JSC::DebuggerCallFrame&, intptr_t sourceID, int lineno);
        virtual void didReachBreakpoint(const JSC::DebuggerCallFrame&, intptr_t sourceID, int lineno);
        
        void didAddListener(Page*);
        void didRemoveListener(Page*);
        void didRemoveLastListener();

        typedef HashMap<Page*, ListenerSet*> PageListenersMap;
        PageListenersMap m_pageListenersMap;
        ListenerSet m_listeners;
        bool m_callingListeners;
        bool m_pauseOnExceptions;
        bool m_pauseOnNextStatement;
        bool m_paused;
        bool m_doneProcessingDebuggerEvents;
        JavaScriptCallFrame* m_pauseOnCallFrame;
        RefPtr<JavaScriptCallFrame> m_currentCallFrame;
        HashMap<intptr_t, HashSet<unsigned>*> m_breakpoints;
        Timer<JavaScriptDebugServer> m_recompileTimer;
    };

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)

#endif // JavaScriptDebugServer_h
