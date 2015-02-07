

#ifndef MessageEvent_h
#define MessageEvent_h

#include "Event.h"
#include "MessagePort.h"

namespace WebCore {

    class DOMWindow;

    class MessageEvent : public Event {
    public:
        static PassRefPtr<MessageEvent> create()
        {
            return adoptRef(new MessageEvent);
        }
        static PassRefPtr<MessageEvent> create(const String& data, const String& origin, const String& lastEventId, PassRefPtr<DOMWindow> source, PassRefPtr<MessagePort> messagePort)
        {
            return adoptRef(new MessageEvent(data, origin, lastEventId, source, messagePort));
        }
        virtual ~MessageEvent();

        void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, const String& data, const String& origin, const String& lastEventId, DOMWindow* source, MessagePort*);
        
        const String& data() const { return m_data; }
        const String& origin() const { return m_origin; }
        const String& lastEventId() const { return m_lastEventId; }
        DOMWindow* source() const { return m_source.get(); }
        MessagePort* messagePort() const { return m_messagePort.get(); }
        
        virtual bool isMessageEvent() const;

    private:    
        MessageEvent();
        MessageEvent(const String& data, const String& origin, const String& lastEventId, PassRefPtr<DOMWindow> source, PassRefPtr<MessagePort> messagePort);

        String m_data;
        String m_origin;
        String m_lastEventId;
        RefPtr<DOMWindow> m_source;
        RefPtr<MessagePort> m_messagePort;
    };

} // namespace WebCore

#endif // MessageEvent_h
