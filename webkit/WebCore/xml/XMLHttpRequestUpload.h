

#ifndef XMLHttpRequestUpload_h
#define XMLHttpRequestUpload_h

#include "AtomicStringHash.h"
#include "EventListener.h"
#include "EventTarget.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class AtomicStringImpl;
    class ScriptExecutionContext;
    class XMLHttpRequest;

    class XMLHttpRequestUpload : public RefCounted<XMLHttpRequestUpload>, public EventTarget {
    public:
        static PassRefPtr<XMLHttpRequestUpload> create(XMLHttpRequest* xmlHttpRequest)
        {
            return adoptRef(new XMLHttpRequestUpload(xmlHttpRequest));
        }

        bool hasListeners() const;

        virtual XMLHttpRequestUpload* toXMLHttpRequestUpload() { return this; }

        XMLHttpRequest* associatedXMLHttpRequest() const { return m_xmlHttpRequest; }
        void disconnectXMLHttpRequest() { m_xmlHttpRequest = 0; }

        ScriptExecutionContext* scriptExecutionContext() const;

        void dispatchAbortEvent();
        void dispatchErrorEvent();
        void dispatchLoadEvent();
        void dispatchLoadStartEvent();
        void dispatchProgressEvent(long long bytesSent, long long totalBytesToBeSent);

        void setOnabort(PassRefPtr<EventListener> eventListener) { m_onAbortListener = eventListener; }
        EventListener* onabort() const { return m_onAbortListener.get(); }

        void setOnerror(PassRefPtr<EventListener> eventListener) { m_onErrorListener = eventListener; }
        EventListener* onerror() const { return m_onErrorListener.get(); }

        void setOnload(PassRefPtr<EventListener> eventListener) { m_onLoadListener = eventListener; }
        EventListener* onload() const { return m_onLoadListener.get(); }

        void setOnloadstart(PassRefPtr<EventListener> eventListener) { m_onLoadStartListener = eventListener; }
        EventListener* onloadstart() const { return m_onLoadStartListener.get(); }

        void setOnprogress(PassRefPtr<EventListener> eventListener) { m_onProgressListener = eventListener; }
        EventListener* onprogress() const { return m_onProgressListener.get(); }

        typedef Vector<RefPtr<EventListener> > ListenerVector;
        typedef HashMap<AtomicString, ListenerVector> EventListenersMap;

        virtual void addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
        virtual void removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
        virtual bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&);
        EventListenersMap& eventListeners() { return m_eventListeners; }

        using RefCounted<XMLHttpRequestUpload>::ref;
        using RefCounted<XMLHttpRequestUpload>::deref;

    private:
        XMLHttpRequestUpload(XMLHttpRequest*);

        void dispatchXMLHttpRequestProgressEvent(EventListener*, const AtomicString& type, bool lengthComputable, unsigned loaded, unsigned total);

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }

        RefPtr<EventListener> m_onAbortListener;
        RefPtr<EventListener> m_onErrorListener;
        RefPtr<EventListener> m_onLoadListener;
        RefPtr<EventListener> m_onLoadStartListener;
        RefPtr<EventListener> m_onProgressListener;
        EventListenersMap m_eventListeners;

        XMLHttpRequest* m_xmlHttpRequest;
    };
    
} // namespace WebCore

#endif // XMLHttpRequestUpload_h
