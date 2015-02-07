

#ifndef Worker_h
#define Worker_h

#if ENABLE(WORKERS)

#include "AbstractWorker.h"
#include "ActiveDOMObject.h"
#include "AtomicStringHash.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "WorkerScriptLoaderClient.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class ScriptExecutionContext;
    class String;
    class WorkerContextProxy;
    class WorkerScriptLoader;

    typedef int ExceptionCode;

    class Worker : public AbstractWorker, private WorkerScriptLoaderClient {
    public:
        static PassRefPtr<Worker> create(const String& url, ScriptExecutionContext* context, ExceptionCode& ec) { return adoptRef(new Worker(url, context, ec)); }
        ~Worker();

        virtual Worker* toWorker() { return this; }

        void postMessage(const String&, ExceptionCode&);
        void postMessage(const String&, MessagePort*, ExceptionCode&);

        void terminate();

        void dispatchMessage(const String&, PassRefPtr<MessagePort>);
        void dispatchErrorEvent();

        virtual bool canSuspend() const;
        virtual void stop();
        virtual bool hasPendingActivity() const;

        void setOnmessage(PassRefPtr<EventListener> eventListener) { m_onMessageListener = eventListener; }
        EventListener* onmessage() const { return m_onMessageListener.get(); }

    private:
        Worker(const String&, ScriptExecutionContext*, ExceptionCode&);

        virtual void notifyFinished();

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }

        OwnPtr<WorkerScriptLoader> m_scriptLoader;

        WorkerContextProxy* m_contextProxy; // The proxy outlives the worker to perform thread shutdown.

        RefPtr<EventListener> m_onMessageListener;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // Worker_h
