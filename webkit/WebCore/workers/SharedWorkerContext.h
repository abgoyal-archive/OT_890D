

#ifndef SharedWorkerContext_h
#define SharedWorkerContext_h

#if ENABLE(SHARED_WORKERS)

#include "WorkerContext.h"

namespace WebCore {

    class SharedWorkerThread;

    class SharedWorkerContext : public WorkerContext {
    public:
        typedef WorkerContext Base;
        static PassRefPtr<SharedWorkerContext> create(const String& name, const KURL& url, const String& userAgent, SharedWorkerThread* thread)
        {
            return adoptRef(new SharedWorkerContext(name, url, userAgent, thread));
        }
        virtual ~SharedWorkerContext();

        virtual bool isSharedWorkerContext() const { return true; }

        // ScriptExecutionContext
        virtual void addMessage(MessageDestination, MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceURL);

        virtual void forwardException(const String& errorMessage, int lineNumber, const String& sourceURL);

        // EventTarget
        virtual SharedWorkerContext* toSharedWorkerContext() { return this; }

        // Setters/Getters for attributes in SharedWorkerContext.idl
        void setOnconnect(PassRefPtr<EventListener> eventListener) { m_onconnectListener = eventListener; }
        EventListener* onconnect() const { return m_onconnectListener.get(); }
        String name() const { return m_name; }

        void dispatchConnect(PassRefPtr<MessagePort>);

        SharedWorkerThread* thread();
    private:
        SharedWorkerContext(const String& name, const KURL&, const String&, SharedWorkerThread*);
        RefPtr<EventListener> m_onconnectListener;
        String m_name;
    };

} // namespace WebCore

#endif // ENABLE(SHARED_WORKERS)

#endif // SharedWorkerContext_h
