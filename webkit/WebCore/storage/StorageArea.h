

#ifndef StorageArea_h
#define StorageArea_h

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"

#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class Frame;
    class SecurityOrigin;
    class StorageSyncManager;
    typedef int ExceptionCode;
    enum StorageType { LocalStorage, SessionStorage };

    // This interface is required for Chromium since these actions need to be proxied between processes.
    class StorageArea : public ThreadSafeShared<StorageArea> {
    public:
        virtual ~StorageArea() { }

        // The HTML5 DOM Storage API
        virtual unsigned length() const = 0;
        virtual String key(unsigned index) const = 0;
        virtual String getItem(const String& key) const = 0;
        virtual void setItem(const String& key, const String& value, ExceptionCode& ec, Frame* sourceFrame) = 0;
        virtual void removeItem(const String& key, Frame* sourceFrame) = 0;
        virtual void clear(Frame* sourceFrame) = 0;
        virtual bool contains(const String& key) const = 0;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageArea_h
