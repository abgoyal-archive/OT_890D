

#ifndef StorageMap_h
#define StorageMap_h

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"
#include "StringHash.h"

#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class StorageMap : public RefCounted<StorageMap> {
    public:
        static PassRefPtr<StorageMap> create();

        unsigned length() const;
        String key(unsigned index) const;
        String getItem(const String&) const;
        PassRefPtr<StorageMap> setItem(const String& key, const String& value, String& oldValue);
        PassRefPtr<StorageMap> removeItem(const String&, String& oldValue);

        bool contains(const String& key) const;

        void importItem(const String& key, const String& value) const;

    private:
        StorageMap();
        PassRefPtr<StorageMap> copy();
        void invalidateIterator();
        void setIteratorToIndex(unsigned) const;

        mutable HashMap<String, String> m_map;
        mutable HashMap<String, String>::iterator m_iterator;
        mutable unsigned m_iteratorIndex;
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageMap_h
