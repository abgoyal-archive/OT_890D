

#ifndef StorageSyncManager_h
#define StorageSyncManager_h

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class LocalStorageThread;
    class SecurityOrigin;
    class StorageAreaSync;

    class StorageSyncManager : public ThreadSafeShared<StorageSyncManager> {
    public:
        static PassRefPtr<StorageSyncManager> create(const String& path);
        ~StorageSyncManager();

        bool scheduleImport(PassRefPtr<StorageAreaSync>);
        void scheduleSync(PassRefPtr<StorageAreaSync>);

        void close();

    private:
        StorageSyncManager(const String& path);

        RefPtr<LocalStorageThread> m_thread;

    // The following members are subject to thread synchronization issues
    public:
        // To be called from the background thread:
        String fullDatabaseFilename(SecurityOrigin*);

    private:
        String m_path;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageSyncManager_h
