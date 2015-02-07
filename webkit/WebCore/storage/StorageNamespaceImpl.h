

#ifndef StorageNamespaceImpl_h
#define StorageNamespaceImpl_h

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"
#include "SecurityOriginHash.h"
#include "StorageArea.h"
#include "StorageNamespace.h"

#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class StorageAreaImpl;

    class StorageNamespaceImpl : public StorageNamespace {
    public:
        static PassRefPtr<StorageNamespace> localStorageNamespace(const String& path);
        static PassRefPtr<StorageNamespace> sessionStorageNamespace();

        virtual ~StorageNamespaceImpl();
        virtual PassRefPtr<StorageArea> storageArea(SecurityOrigin*);
        virtual PassRefPtr<StorageNamespace> copy();
        virtual void close();

    private:
        StorageNamespaceImpl(StorageType, const String& path);

        typedef HashMap<RefPtr<SecurityOrigin>, RefPtr<StorageAreaImpl>, SecurityOriginHash> StorageAreaMap;
        StorageAreaMap m_storageAreaMap;

        StorageType m_storageType;

        // Only used if m_storageType == LocalStorage and the path was not "" in our constructor.
        String m_path;
        RefPtr<StorageSyncManager> m_syncManager;

        bool m_isShutdown;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageNamespaceImpl_h
