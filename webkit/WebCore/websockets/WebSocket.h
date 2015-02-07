

#ifndef WebSocket_h
#define WebSocket_h

#include "ActiveDOMObject.h"
#include "AtomicStringHash.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "KURL.h"
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class String;

class WebSocket : public RefCounted<WebSocket>, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<WebSocket> create(ScriptExecutionContext* context) { return adoptRef(new WebSocket(context)); }
    virtual ~WebSocket();

    enum State {
        CONNECTING = 0,
        OPEN = 1,
        CLOSED = 2
    };

    void connect(const KURL& url, ExceptionCode&);
    void connect(const KURL& url, const String& protocol, ExceptionCode&);

    bool send(const String& message, ExceptionCode&);

    void close();

    const KURL& url() const;
    State readyState() const;
    unsigned long bufferedAmount() const;

    void setOnopen(PassRefPtr<EventListener> eventListener) { m_onopen = eventListener; }
    EventListener* onopen() const { return m_onopen.get(); }
    void setOnmessage(PassRefPtr<EventListener> eventListener) { m_onmessage = eventListener; }
    EventListener* onmessage() const { return m_onmessage.get(); }
    void setOnclose(PassRefPtr<EventListener> eventListener) { m_onclose = eventListener; }
    EventListener* onclose() const { return m_onclose.get(); }

    // EventTarget
    virtual WebSocket* toWebSocket() { return this; }

    virtual ScriptExecutionContext* scriptExecutionContext() const;

    virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
    virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
    virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&);

    // ActiveDOMObject
    // virtual bool hasPendingActivity() const;
    // virtual void contextDestroyed();
    // virtual bool canSuspend() const;
    // virtual void suspend();
    // virtual void resume();
    // virtual void stop();

    using RefCounted<WebSocket>::ref;
    using RefCounted<WebSocket>::deref;

private:
    WebSocket(ScriptExecutionContext*);

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }

    // WebSocketChannelClient
    void didConnect();
    void didReceiveMessage(const String& msg);
    void didClose();

    void dispatchOpenEvent();
    void dispatchMessageEvent(const String& msg);
    void dispatchCloseEvent();

    // FIXME: add WebSocketChannel.

    RefPtr<EventListener> m_onopen;
    RefPtr<EventListener> m_onmessage;
    RefPtr<EventListener> m_onclose;

    State m_state;
    KURL m_url;
    String m_protocol;
};

}  // namespace WebCore

#endif  // WebSocket_h
