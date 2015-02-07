

#ifndef WorkerContextProxy_h
#define WorkerContextProxy_h

#if ENABLE(WORKERS)

#include <wtf/PassOwnPtr.h>

namespace WebCore {

    class KURL;
    class MessagePortChannel;
    class String;
    class Worker;

    // A proxy to talk to the worker context.
    class WorkerContextProxy {
    public:
        static WorkerContextProxy* create(Worker*);

        virtual ~WorkerContextProxy() {}

        virtual void startWorkerContext(const KURL& scriptURL, const String& userAgent, const String& sourceCode) = 0;

        virtual void terminateWorkerContext() = 0;

        virtual void postMessageToWorkerContext(const String&, PassOwnPtr<MessagePortChannel>) = 0;

        virtual bool hasPendingActivity() const = 0;

        virtual void workerObjectDestroyed() = 0;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerContextProxy_h
