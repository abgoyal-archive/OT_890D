

#ifndef StorageEvent_h
#define StorageEvent_h

#if ENABLE(DOM_STORAGE)

#include "Event.h"
#include "PlatformString.h"

namespace WebCore {

    class DOMWindow;
    class Storage;

    class StorageEvent : public Event {
    public:
        static PassRefPtr<StorageEvent> create();
        static PassRefPtr<StorageEvent> create(const AtomicString& type, const String& key, const String& oldValue, const String& newValue, const String& uri, PassRefPtr<DOMWindow> source, Storage* storageArea);

        const String& key() const { return m_key; }
        const String& oldValue() const { return m_oldValue; }
        const String& newValue() const { return m_newValue; }
        const String& uri() const { return m_uri; }
        DOMWindow* source() const { return m_source.get(); }
        Storage* storageArea() const { return m_storageArea.get(); }

        void initStorageEvent(const AtomicString& type, bool canBubble, bool cancelable, const String& key, const String& oldValue, const String& newValue, const String& uri, PassRefPtr<DOMWindow> source, Storage* storageArea);

        // Needed once we support init<blank>EventNS
        // void initStorageEventNS(in DOMString namespaceURI, in DOMString typeArg, in boolean canBubbleArg, in boolean cancelableArg, in DOMString keyArg, in DOMString oldValueArg, in DOMString newValueArg, in DOMString uriArg, in Window sourceArg,  Storage storageAreaArg);

        virtual bool isStorageEvent() const { return true; }

    private:    
        StorageEvent();
        StorageEvent(const AtomicString& type, const String& key, const String& oldValue, const String& newValue, const String& uri, PassRefPtr<DOMWindow> source, Storage* storageArea);
        
        String m_key;
        String m_oldValue;
        String m_newValue;
        String m_uri;
        RefPtr<DOMWindow> m_source;
        RefPtr<Storage> m_storageArea;        
    };

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)

#endif // StorageEvent_h
