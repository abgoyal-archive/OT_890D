

#ifndef EventTarget_h
#define EventTarget_h

#include <wtf/Forward.h>

namespace WebCore {

    class AbstractWorker;
    class AtomicString;
    class DedicatedWorkerContext;
    class DOMApplicationCache;
    class DOMWindow;
    class Event;
    class EventListener;
    class MessagePort;
    class Node;
    class SVGElementInstance;
    class ScriptExecutionContext;
    class SharedWorker;
    class SharedWorkerContext;
    class Worker;
    class XMLHttpRequest;
    class XMLHttpRequestUpload;

    typedef int ExceptionCode;

    class EventTarget {
    public:
        virtual MessagePort* toMessagePort();
        virtual Node* toNode();
        virtual DOMWindow* toDOMWindow();
        virtual XMLHttpRequest* toXMLHttpRequest();
        virtual XMLHttpRequestUpload* toXMLHttpRequestUpload();
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
        virtual DOMApplicationCache* toDOMApplicationCache();
#endif
#if ENABLE(SVG)
        virtual SVGElementInstance* toSVGElementInstance();
#endif
#if ENABLE(WORKERS)
        virtual Worker* toWorker();
        virtual DedicatedWorkerContext* toDedicatedWorkerContext();
#endif

#if ENABLE(SHARED_WORKERS)
        virtual SharedWorker* toSharedWorker();
        virtual SharedWorkerContext* toSharedWorkerContext();
#endif

        virtual ScriptExecutionContext* scriptExecutionContext() const = 0;

        virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture) = 0;
        virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture) = 0;
        virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&) = 0;

        void ref() { refEventTarget(); }
        void deref() { derefEventTarget(); }

        // Handlers to do/undo actions on the target node before an event is dispatched to it and after the event
        // has been dispatched.  The data pointer is handed back by the preDispatch and passed to postDispatch.
        virtual void* preDispatchEventHandler(Event*) { return 0; }
        virtual void postDispatchEventHandler(Event*, void* /*dataFromPreDispatch*/) { }

    protected:
        virtual ~EventTarget();

    private:
        virtual void refEventTarget() = 0;
        virtual void derefEventTarget() = 0;
    };

    void forbidEventDispatch();
    void allowEventDispatch();

#ifndef NDEBUG
    bool eventDispatchForbidden();
#else
    inline void forbidEventDispatch() { }
    inline void allowEventDispatch() { }
#endif

}

#endif
