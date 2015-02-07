

#ifndef EventListener_h
#define EventListener_h

#include "PlatformString.h"
#include <wtf/RefCounted.h>

namespace JSC {
    class JSObject;
    class MarkStack;
}

namespace WebCore {

    class Event;

    class EventListener : public RefCounted<EventListener> {
    public:
        virtual ~EventListener() { }
        virtual void handleEvent(Event*, bool isWindowEvent = false) = 0;
        // Return true to indicate that the error is handled.
        virtual bool reportError(const String& /*message*/, const String& /*url*/, int /*lineNumber*/) { return false; }
        virtual bool wasCreatedFromMarkup() const { return false; }

#if USE(JSC)
        virtual JSC::JSObject* jsFunction() const { return 0; }
        virtual void markJSFunction(JSC::MarkStack&) { }
#endif

        bool isAttribute() const { return virtualisAttribute(); }

    private:
        virtual bool virtualisAttribute() const { return false; }
    };

#if USE(JSC)
    inline void markIfNotNull(JSC::MarkStack& markStack, EventListener* listener) { if (listener) listener->markJSFunction(markStack); }
#endif

}

#endif
