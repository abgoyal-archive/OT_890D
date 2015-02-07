

#ifndef V8ObjectEventListener_h
#define V8ObjectEventListener_h

#include "V8CustomEventListener.h"
#include <v8.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Frame;

    // V8ObjectEventListener is a special listener wrapper for objects not in the DOM.  It keeps the JS listener as a weak pointer.
    class V8ObjectEventListener : public V8EventListener {
    public:
        static PassRefPtr<V8ObjectEventListener> create(Frame* frame, v8::Local<v8::Object> listener, bool isInline)
        {
            return adoptRef(new V8ObjectEventListener(frame, listener, isInline));
        }

        virtual bool isObjectListener() const { return true; }

    private:
        V8ObjectEventListener(Frame*, v8::Local<v8::Object> listener, bool isInline);
        virtual ~V8ObjectEventListener();
    };

} // namespace WebCore

#endif // V8ObjectEventListener_h
