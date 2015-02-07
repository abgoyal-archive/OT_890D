

#ifndef LocalStorageTask_h
#define LocalStorageTask_h

#if ENABLE(DOM_STORAGE)

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class StorageAreaSync;
    class LocalStorageThread;

    // FIXME: Rename this class to StorageTask
    class LocalStorageTask : public ThreadSafeShared<LocalStorageTask> {
    public:
        enum Type { AreaImport, AreaSync, TerminateThread };

        ~LocalStorageTask();

        static PassRefPtr<LocalStorageTask> createImport(PassRefPtr<StorageAreaSync> area) { return adoptRef(new LocalStorageTask(AreaImport, area)); }
        static PassRefPtr<LocalStorageTask> createSync(PassRefPtr<StorageAreaSync> area) { return adoptRef(new LocalStorageTask(AreaSync, area)); }
        static PassRefPtr<LocalStorageTask> createTerminate(PassRefPtr<LocalStorageThread> thread) { return adoptRef(new LocalStorageTask(TerminateThread, thread)); }

        void performTask();

    private:
        LocalStorageTask(Type, PassRefPtr<StorageAreaSync>);
        LocalStorageTask(Type, PassRefPtr<LocalStorageThread>);

        Type m_type;
        RefPtr<StorageAreaSync> m_area;
        RefPtr<LocalStorageThread> m_thread;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // LocalStorageTask_h
