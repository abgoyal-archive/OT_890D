

#ifndef DedicatedWorkerContext_h
#define DedicatedWorkerContext_h

#if ENABLE(WORKERS)

#include "WorkerContext.h"

namespace WebCore {

    class DedicatedWorkerThread;

    class DedicatedWorkerContext : public WorkerContext {
    public:
        typedef WorkerContext Base;
        static PassRefPtr<DedicatedWorkerContext> create(const KURL& url, const String& userAgent, DedicatedWorkerThread* thread)
        {
            return adoptRef(new DedicatedWorkerContext(url, userAgent, thread));
        }
        virtual ~DedicatedWorkerContext();

        virtual bool isDedicatedWorkerContext() const { return true; }

        // Overridden to allow us to check our pending activity after executing imported script.
        virtual void importScripts(const Vector<String>& urls, const String& callerURL, int callerLine, ExceptionCode&);

        // ScriptExecutionContext
        virtual void addMessage(MessageDestination, MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceURL);

        virtual void forwardException(const String& errorMessage, int lineNumber, const String& sourceURL);

        // EventTarget
        virtual DedicatedWorkerContext* toDedicatedWorkerContext() { return this; }
        void postMessage(const String&, ExceptionCode&);
        void postMessage(const String&, MessagePort*, ExceptionCode&);
        void setOnmessage(PassRefPtr<EventListener> eventListener) { m_onmessageListener = eventListener; }
        EventListener* onmessage() const { return m_onmessageListener.get(); }

        void dispatchMessage(const String&, PassRefPtr<MessagePort>);

        DedicatedWorkerThread* thread();
    private:
        DedicatedWorkerContext(const KURL&, const String&, DedicatedWorkerThread*);
        RefPtr<EventListener> m_onmessageListener;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // DedicatedWorkerContext_h
