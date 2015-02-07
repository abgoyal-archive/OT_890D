

#ifndef StorageAreaImpl_h
#define StorageAreaImpl_h

#if ENABLE(DOM_STORAGE)

#include "StorageArea.h"

#include <wtf/RefPtr.h>

namespace WebCore {

    class SecurityOrigin;
    class StorageMap;
    class StorageAreaSync;

    class StorageAreaImpl : public StorageArea {
    public:
        StorageAreaImpl(StorageType, PassRefPtr<SecurityOrigin>, PassRefPtr<StorageSyncManager>);
        virtual ~StorageAreaImpl();

        // The HTML5 DOM Storage API (and contains)
        virtual unsigned length() const;
        virtual String key(unsigned index) const;
        virtual String getItem(const String& key) const;
        virtual void setItem(const String& key, const String& value, ExceptionCode& ec, Frame* sourceFrame);
        virtual void removeItem(const String& key, Frame* sourceFrame);
        virtual void clear(Frame* sourceFrame);
        virtual bool contains(const String& key) const;

        PassRefPtr<StorageAreaImpl> copy();
        void close();

        // Could be called from a background thread.
        void importItem(const String& key, const String& value);
        SecurityOrigin* securityOrigin();

    private:
        StorageAreaImpl(StorageAreaImpl*);

        void blockUntilImportComplete() const;

        void dispatchStorageEvent(const String& key, const String& oldValue, const String& newValue, Frame* sourceFrame);

        StorageType m_storageType;
        RefPtr<SecurityOrigin> m_securityOrigin;
        RefPtr<StorageMap> m_storageMap;

        RefPtr<StorageAreaSync> m_storageAreaSync;
        RefPtr<StorageSyncManager> m_storageSyncManager;

#ifndef NDEBUG
        bool m_isShutdown;
#endif
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageAreaImpl_h
