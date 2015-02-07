

#ifndef WorkerObjectProxy_h
#define WorkerObjectProxy_h

#if ENABLE(WORKERS)

#include "Console.h"

#include <wtf/PassOwnPtr.h>

namespace WebCore {

    class MessagePortChannel;
    class String;

    // A proxy to talk to the worker object.
    class WorkerObjectProxy {
    public:
        virtual ~WorkerObjectProxy() {}

        virtual void postMessageToWorkerObject(const String&, PassOwnPtr<MessagePortChannel>) = 0;

        virtual void postExceptionToWorkerObject(const String& errorMessage, int lineNumber, const String& sourceURL) = 0;

        virtual void postConsoleMessageToWorkerObject(MessageDestination, MessageSource, MessageType, MessageLevel, const String& message, int lineNumber, const String& sourceURL) = 0;

        virtual void confirmMessageFromWorkerObject(bool hasPendingActivity) = 0;

        virtual void reportPendingActivity(bool hasPendingActivity) = 0;

        virtual void workerContextDestroyed() = 0;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerObjectProxy_h
