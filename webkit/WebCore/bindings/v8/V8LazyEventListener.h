

#ifndef V8LazyEventListener_h
#define V8LazyEventListener_h

#include "PlatformString.h"
#include "V8AbstractEventListener.h"
#include <v8.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Event;
    class Frame;

    // V8LazyEventListener is a wrapper for a JavaScript code string that is compiled and evaluated when an event is fired.
    // A V8LazyEventListener is always a HTML event handler.
    class V8LazyEventListener : public V8AbstractEventListener {
    public:
        static PassRefPtr<V8LazyEventListener> create(Frame* frame, const String& code, const String& functionName, bool isSVGEvent)
        {
            return adoptRef(new V8LazyEventListener(frame, code, functionName, isSVGEvent));
        }

        // For lazy event listener, the listener object is the same as its listener
        // function without additional scope chains.
        virtual v8::Local<v8::Object> getListenerObject() { return getWrappedListenerFunction(); }

    private:
        V8LazyEventListener(Frame*, const String& code, const String& functionName, bool isSVGEvent);
        virtual ~V8LazyEventListener();

        virtual bool virtualisAttribute() const { return true; }

        String m_code;
        String m_functionName;
        bool m_isSVGEvent;
        bool m_compiled;

        // If the event listener is on a non-document dom node, we compile the function with some implicit scope chains before it.
        bool m_wrappedFunctionCompiled;
        v8::Persistent<v8::Function> m_wrappedFunction;

        v8::Local<v8::Function> getWrappedListenerFunction();

        virtual v8::Local<v8::Value> callListenerFunction(v8::Handle<v8::Value> jsEvent, Event*, bool isWindowEvent);

        v8::Local<v8::Function> getListenerFunction();
    };

} // namespace WebCore

#endif // V8LazyEventListener_h
