

#ifndef LocalStorageThread_h
#define LocalStorageThread_h

#if ENABLE(DOM_STORAGE)

#include <wtf/HashSet.h>
#include <wtf/MessageQueue.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class StorageAreaSync;
    class LocalStorageTask;

    // FIXME: Rename this class to StorageThread
    class LocalStorageThread : public ThreadSafeShared<LocalStorageThread> {
    public:
        static PassRefPtr<LocalStorageThread> create();

        bool start();

        void scheduleImport(PassRefPtr<StorageAreaSync>);
        void scheduleSync(PassRefPtr<StorageAreaSync>);

        // Called from the main thread to synchronously shut down this thread
        void terminate();
        // Background thread part of the terminate procedure
        void performTerminate();

    private:
        LocalStorageThread();

        static void* localStorageThreadStart(void*);
        void* localStorageThread();

        Mutex m_threadCreationMutex;
        ThreadIdentifier m_threadID;
        RefPtr<LocalStorageThread> m_selfRef;

        MessageQueue<RefPtr<LocalStorageTask> > m_queue;
        
        Mutex m_terminateLock;
        ThreadCondition m_terminateCondition;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // LocalStorageThread_h
