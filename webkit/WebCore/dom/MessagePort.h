

#ifndef MessagePort_h
#define MessagePort_h

#include "AtomicStringHash.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "MessagePortChannel.h"

#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class AtomicStringImpl;
    class Event;
    class Frame;
    class ScriptExecutionContext;
    class String;

    class MessagePort : public RefCounted<MessagePort>, public EventTarget {
    public:
        static PassRefPtr<MessagePort> create(ScriptExecutionContext& scriptExecutionContext) { return adoptRef(new MessagePort(scriptExecutionContext)); }
        ~MessagePort();

        void postMessage(const String& message, ExceptionCode&);
        void postMessage(const String& message, MessagePort*, ExceptionCode&);
        void start();
        void close();

        void entangle(PassOwnPtr<MessagePortChannel>);
        PassOwnPtr<MessagePortChannel> disentangle(ExceptionCode&);

        void messageAvailable();
        bool started() const { return m_started; }

        void contextDestroyed();

        virtual ScriptExecutionContext* scriptExecutionContext() const;

        virtual MessagePort* toMessagePort() { return this; }

        void dispatchMessages();

        virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
        virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
        virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&);

        typedef Vector<RefPtr<EventListener> > ListenerVector;
        typedef HashMap<AtomicString, ListenerVector> EventListenersMap;
        EventListenersMap& eventListeners() { return m_eventListeners; }

        using RefCounted<MessagePort>::ref;
        using RefCounted<MessagePort>::deref;

        bool hasPendingActivity();

        void setOnmessage(PassRefPtr<EventListener>);
        EventListener* onmessage() const { return m_onMessageListener.get(); }

        // Returns null if there is no entangled port, or if the entangled port is run by a different thread.
        // Returns null otherwise.
        // NOTE: This is used solely to enable a GC optimization. Some platforms may not be able to determine ownership of the remote port (since it may live cross-process) - those platforms may always return null.
        MessagePort* locallyEntangledPort();
        bool isEntangled() { return m_entangledChannel; }

    private:
        MessagePort(ScriptExecutionContext&);

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }

        OwnPtr<MessagePortChannel> m_entangledChannel;

        bool m_started;

        ScriptExecutionContext* m_scriptExecutionContext;

        RefPtr<EventListener> m_onMessageListener;

        EventListenersMap m_eventListeners;
    };

} // namespace WebCore

#endif // MessagePort_h
