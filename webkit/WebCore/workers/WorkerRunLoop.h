

#ifndef WorkerRunLoop_h
#define WorkerRunLoop_h

#if ENABLE(WORKERS)

#include "ScriptExecutionContext.h"
#include <wtf/MessageQueue.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class ModePredicate;
    class WorkerContext;
    class WorkerSharedTimer;

    class WorkerRunLoop {
    public:
        WorkerRunLoop();
        ~WorkerRunLoop();
        
        // Blocking call. Waits for tasks and timers, invokes the callbacks.
        void run(WorkerContext*);

        // Waits for a single task and returns.
        MessageQueueWaitResult runInMode(WorkerContext*, const String& mode);

        void terminate();
        bool terminated() { return m_messageQueue.killed(); }

        void postTask(PassRefPtr<ScriptExecutionContext::Task>);
        void postTaskForMode(PassRefPtr<ScriptExecutionContext::Task>, const String& mode);

        unsigned long createUniqueId() { return ++m_uniqueId; }

        static String defaultMode();
        class Task;
    private:
        friend class RunLoopSetup;
        MessageQueueWaitResult runInMode(WorkerContext*, const ModePredicate&);

        MessageQueue<RefPtr<Task> > m_messageQueue;
        OwnPtr<WorkerSharedTimer> m_sharedTimer;
        int m_nestedCount;
        unsigned long m_uniqueId;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerRunLoop_h
