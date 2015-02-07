

#ifndef DOMApplicationCache_h
#define DOMApplicationCache_h

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCacheHost.h"
#include "AtomicStringHash.h"
#include "EventTarget.h"
#include "EventListener.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class AtomicStringImpl;
class Frame;
class KURL;
class String;

class DOMApplicationCache : public RefCounted<DOMApplicationCache>, public EventTarget {
public:
    static PassRefPtr<DOMApplicationCache> create(Frame* frame) { return adoptRef(new DOMApplicationCache(frame)); }
    ~DOMApplicationCache() { ASSERT(!m_frame); }

    void disconnectFrame();

    unsigned short status() const;
    void update(ExceptionCode&);
    void swapCache(ExceptionCode&);

    // Event listener attributes by EventID

    void setAttributeEventListener(ApplicationCacheHost::EventID id, PassRefPtr<EventListener> eventListener) { m_attributeEventListeners[id] = eventListener; }
    EventListener* getAttributeEventListener(ApplicationCacheHost::EventID id) const { return m_attributeEventListeners[id].get(); }
    void clearAttributeEventListener(ApplicationCacheHost::EventID id) { m_attributeEventListeners[id] = 0; }
    void callEventListener(ApplicationCacheHost::EventID id) { callListener(toEventType(id), getAttributeEventListener(id)); }

    // EventTarget impl

    virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
    virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
    virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&);
    typedef Vector<RefPtr<EventListener> > ListenerVector;
    typedef HashMap<AtomicString, ListenerVector> EventListenersMap;
    EventListenersMap& eventListeners() { return m_eventListeners; }

    using RefCounted<DOMApplicationCache>::ref;
    using RefCounted<DOMApplicationCache>::deref;

    // Explicitly named attribute event listener helpers

    void setOnchecking(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::CHECKING_EVENT, listener); }
    EventListener* onchecking() const { return getAttributeEventListener(ApplicationCacheHost::CHECKING_EVENT); }

    void setOnerror(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::ERROR_EVENT, listener);}
    EventListener* onerror() const { return getAttributeEventListener(ApplicationCacheHost::ERROR_EVENT); }

    void setOnnoupdate(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::NOUPDATE_EVENT, listener); }
    EventListener* onnoupdate() const { return getAttributeEventListener(ApplicationCacheHost::NOUPDATE_EVENT); }

    void setOndownloading(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::DOWNLOADING_EVENT, listener); }
    EventListener* ondownloading() const { return getAttributeEventListener(ApplicationCacheHost::DOWNLOADING_EVENT); }

    void setOnprogress(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::PROGRESS_EVENT, listener); }
    EventListener* onprogress() const { return getAttributeEventListener(ApplicationCacheHost::PROGRESS_EVENT); }

    void setOnupdateready(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::UPDATEREADY_EVENT, listener); }
    EventListener* onupdateready() const { return getAttributeEventListener(ApplicationCacheHost::UPDATEREADY_EVENT); }

    void setOncached(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::CACHED_EVENT, listener); }
    EventListener* oncached() const { return getAttributeEventListener(ApplicationCacheHost::CACHED_EVENT); }

    void setOnobsolete(PassRefPtr<EventListener> listener) { setAttributeEventListener(ApplicationCacheHost::OBSOLETE_EVENT, listener); }
    EventListener* onobsolete() const { return getAttributeEventListener(ApplicationCacheHost::OBSOLETE_EVENT); }

    virtual ScriptExecutionContext* scriptExecutionContext() const;
    DOMApplicationCache* toDOMApplicationCache() { return this; }

    static const AtomicString& toEventType(ApplicationCacheHost::EventID id);
    static ApplicationCacheHost::EventID toEventID(const AtomicString& eventType);

private:
    DOMApplicationCache(Frame*);

    void callListener(const AtomicString& eventType, EventListener*);
    
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }

    ApplicationCacheHost* applicationCacheHost() const;
    bool swapCache();
    
    RefPtr<EventListener> m_attributeEventListeners[ApplicationCacheHost::OBSOLETE_EVENT + 1];

    EventListenersMap m_eventListeners;

    Frame* m_frame;
};

} // namespace WebCore

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)

#endif // DOMApplicationCache_h
