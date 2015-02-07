

#ifndef V8WorkerContextObjectEventListener_h
#define V8WorkerContextObjectEventListener_h

#if ENABLE(WORKERS)

#include "V8WorkerContextEventListener.h"
#include <v8.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class WorkerContextExecutionProxy;

    class V8WorkerContextObjectEventListener : public V8WorkerContextEventListener {
    public:
        static PassRefPtr<V8WorkerContextObjectEventListener> create(WorkerContextExecutionProxy* proxy, v8::Local<v8::Object> listener, bool isInline)
        {
            return adoptRef(new V8WorkerContextObjectEventListener(proxy, listener, isInline));
        }

    private:
        V8WorkerContextObjectEventListener(WorkerContextExecutionProxy*, v8::Local<v8::Object> listener, bool isInline);
    };

} // namespace WebCore

#endif // WORKERS

#endif // V8WorkerContextObjectEventListener_h
