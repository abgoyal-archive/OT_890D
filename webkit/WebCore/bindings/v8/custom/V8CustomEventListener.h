

#ifndef V8CustomEventListener_h
#define V8CustomEventListener_h

#include "V8AbstractEventListener.h"
#include <v8.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Event;
    class Frame;

    // V8EventListener is a wrapper of a JS object implements EventListener interface (has handleEvent(event) method), or a JS function
    // that can handle the event.
    class V8EventListener : public V8AbstractEventListener {
    public:
        static PassRefPtr<V8EventListener> create(Frame* frame, v8::Local<v8::Object> listener, bool isAttribute)
        {
            return adoptRef(new V8EventListener(frame, listener, isAttribute));
        }

        // Detach the listener from its owner frame.
        void disconnectFrame() { m_frame = 0; }

    protected:
        V8EventListener(Frame*, v8::Local<v8::Object> listener, bool isAttribute);
        virtual ~V8EventListener();
        v8::Local<v8::Function> getListenerFunction();

    private:
        virtual v8::Local<v8::Value> callListenerFunction(v8::Handle<v8::Value> jsEvent, Event*, bool isWindowEvent);
        virtual bool virtualisAttribute() const { return m_isAttribute; }
    };

} // namespace WebCore

#endif // V8CustomEventListener_h
