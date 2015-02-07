

#ifndef RegisteredEventListener_h
#define RegisteredEventListener_h

#include "AtomicString.h"
#include "EventListener.h"

namespace WebCore {

    class RegisteredEventListener : public RefCounted<RegisteredEventListener> {
    public:
        static PassRefPtr<RegisteredEventListener> create(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
        {
            return adoptRef(new RegisteredEventListener(eventType, listener, useCapture));
        }

        const AtomicString& eventType() const { return m_eventType; }
        EventListener* listener() const { return m_listener.get(); }
        bool useCapture() const { return m_useCapture; }
        
        bool removed() const { return m_removed; }
        void setRemoved(bool removed) { m_removed = removed; }
    
    private:
        RegisteredEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);

        AtomicString m_eventType;
        RefPtr<EventListener> m_listener;
        bool m_useCapture;
        bool m_removed;
    };

    typedef Vector<RefPtr<RegisteredEventListener> > RegisteredEventListenerVector;

#if USE(JSC)
    inline void markEventListeners(JSC::MarkStack& markStack, const RegisteredEventListenerVector& listeners)
    {
        for (size_t i = 0; i < listeners.size(); ++i)
            listeners[i]->listener()->markJSFunction(markStack);
    }

    inline void invalidateEventListeners(const RegisteredEventListenerVector& listeners)
    {
        // For efficiency's sake, we just set the "removed" bit, instead of
        // actually removing the event listener. The node that owns these
        // listeners is about to be deleted, anyway.
        for (size_t i = 0; i < listeners.size(); ++i)
            listeners[i]->setRemoved(true);
    }
#endif

} // namespace WebCore

#endif // RegisteredEventListener_h
