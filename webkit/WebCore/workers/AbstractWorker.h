

#ifndef AbstractWorker_h
#define AbstractWorker_h

#if ENABLE(WORKERS)

#include "ActiveDOMObject.h"
#include "AtomicStringHash.h"
#include "EventListener.h"
#include "EventTarget.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class KURL;
    class ScriptExecutionContext;

    class AbstractWorker : public RefCounted<AbstractWorker>, public ActiveDOMObject, public EventTarget {
    public:
        // EventTarget APIs
        virtual ScriptExecutionContext* scriptExecutionContext() const { return ActiveDOMObject::scriptExecutionContext(); }

        virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
        virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
        virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&);

        // Utility routines to generate appropriate error events for loading and script exceptions.
        void dispatchLoadErrorEvent();
        bool dispatchScriptErrorEvent(const String& errorMessage, const String& sourceURL, int);

        void setOnerror(PassRefPtr<EventListener> eventListener) { m_onErrorListener = eventListener; }
        EventListener* onerror() const { return m_onErrorListener.get(); }
        typedef Vector<RefPtr<EventListener> > ListenerVector;
        typedef HashMap<AtomicString, ListenerVector> EventListenersMap;
        EventListenersMap& eventListeners() { return m_eventListeners; }

        using RefCounted<AbstractWorker>::ref;
        using RefCounted<AbstractWorker>::deref;

        AbstractWorker(ScriptExecutionContext*);
        virtual ~AbstractWorker();

    protected:
        // Helper function that converts a URL to an absolute URL and checks the result for validity.
        KURL resolveURL(const String& url, ExceptionCode& ec);

    private:
        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }

        RefPtr<EventListener> m_onErrorListener;
        EventListenersMap m_eventListeners;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // AbstractWorker_h
