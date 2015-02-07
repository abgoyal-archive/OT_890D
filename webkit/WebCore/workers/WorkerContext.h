

#ifndef WorkerContext_h
#define WorkerContext_h

#if ENABLE(WORKERS)

#include "AtomicStringHash.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "ScriptExecutionContext.h"
#include "WorkerScriptController.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class ScheduledAction;
    class WorkerLocation;
    class WorkerNavigator;
    class WorkerThread;

    class WorkerContext : public RefCounted<WorkerContext>, public ScriptExecutionContext, public EventTarget {
    public:

        virtual ~WorkerContext();

        virtual bool isWorkerContext() const { return true; }

        virtual ScriptExecutionContext* scriptExecutionContext() const;

        virtual bool isSharedWorkerContext() const { return false; }
        virtual bool isDedicatedWorkerContext() const { return false; }

        const KURL& url() const { return m_url; }
        KURL completeURL(const String&) const;

        virtual String userAgent(const KURL&) const;

        WorkerScriptController* script() { return m_script.get(); }
        void clearScript() { return m_script.clear(); }

        WorkerThread* thread() { return m_thread; }

        bool hasPendingActivity() const;

        virtual void resourceRetrievedByXMLHttpRequest(unsigned long identifier, const ScriptString& sourceString);
        virtual void scriptImported(unsigned long identifier, const String& sourceString);

        virtual void postTask(PassRefPtr<Task>); // Executes the task on context's thread asynchronously.

        // WorkerGlobalScope
        WorkerContext* self() { return this; }
        WorkerLocation* location() const;
        void close();
        void setOnerror(PassRefPtr<EventListener> eventListener) { m_onerrorListener = eventListener; }
        EventListener* onerror() const { return m_onerrorListener.get(); }

        // WorkerUtils
        virtual void importScripts(const Vector<String>& urls, const String& callerURL, int callerLine, ExceptionCode&);
        WorkerNavigator* navigator() const;

        // Timers
        int setTimeout(ScheduledAction*, int timeout);
        void clearTimeout(int timeoutId);
        int setInterval(ScheduledAction*, int timeout);
        void clearInterval(int timeoutId);

        // EventTarget
        virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
        virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
        virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&);

        typedef Vector<RefPtr<EventListener> > ListenerVector;
        typedef HashMap<AtomicString, ListenerVector> EventListenersMap;
        EventListenersMap& eventListeners() { return m_eventListeners; }

        // ScriptExecutionContext
        virtual void reportException(const String& errorMessage, int lineNumber, const String& sourceURL);

        virtual void forwardException(const String& errorMessage, int lineNumber, const String& sourceURL) = 0;

        // These methods are used for GC marking. See JSWorkerContext::markChildren(MarkStack&) in
        // JSWorkerContextCustom.cpp.
        WorkerNavigator* optionalNavigator() const { return m_navigator.get(); }
        WorkerLocation* optionalLocation() const { return m_location.get(); }

        using RefCounted<WorkerContext>::ref;
        using RefCounted<WorkerContext>::deref;

    protected:
        WorkerContext(const KURL&, const String&, WorkerThread*);
        bool isClosing() { return m_closing; }

    private:
        virtual void refScriptExecutionContext() { ref(); }
        virtual void derefScriptExecutionContext() { deref(); }
        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }

        virtual const KURL& virtualURL() const;
        virtual KURL virtualCompleteURL(const String&) const;

        KURL m_url;
        String m_userAgent;

        mutable RefPtr<WorkerLocation> m_location;
        mutable RefPtr<WorkerNavigator> m_navigator;

        OwnPtr<WorkerScriptController> m_script;
        WorkerThread* m_thread;

        RefPtr<EventListener> m_onerrorListener;
        EventListenersMap m_eventListeners;

        bool m_closing;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerContext_h
