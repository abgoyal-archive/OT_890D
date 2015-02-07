

#ifndef V8WorkerContextEventListener_h
#define V8WorkerContextEventListener_h

#if ENABLE(WORKERS)

#include "V8CustomEventListener.h"
#include <v8.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Event;
    class WorkerContextExecutionProxy;

    class V8WorkerContextEventListener : public V8EventListener {
    public:
        static PassRefPtr<V8WorkerContextEventListener> create(WorkerContextExecutionProxy* proxy, v8::Local<v8::Object> listener, bool isInline)
        {
            return adoptRef(new V8WorkerContextEventListener(proxy, listener, isInline));
        }
        V8WorkerContextEventListener(WorkerContextExecutionProxy*, v8::Local<v8::Object> listener, bool isInline);

        virtual ~V8WorkerContextEventListener();
        virtual void handleEvent(Event*, bool isWindowEvent);
        virtual bool reportError(const String& message, const String& url, int lineNumber);
        virtual bool disconnected() const { return !m_proxy; }

        WorkerContextExecutionProxy* proxy() const { return m_proxy; }
        void disconnect() { m_proxy = 0; }

    private:
        virtual v8::Local<v8::Value> callListenerFunction(v8::Handle<v8::Value> jsEvent, Event*, bool isWindowEvent);
        v8::Local<v8::Object> getReceiverObject(Event*, bool isWindowEvent);
        WorkerContextExecutionProxy* m_proxy;
    };

} // namespace WebCore

#endif // WORKERS

#endif // V8WorkerContextEventListener_h
