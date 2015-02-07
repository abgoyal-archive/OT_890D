

#ifndef ObjCEventListener_h
#define ObjCEventListener_h

#include "EventListener.h"

#include <wtf/PassRefPtr.h>

@protocol DOMEventListener;

namespace WebCore {

    class ObjCEventListener : public EventListener {
    public:
        static PassRefPtr<ObjCEventListener> wrap(id <DOMEventListener>);

    private:
        static ObjCEventListener* find(id <DOMEventListener>);

        ObjCEventListener(id <DOMEventListener>);
        virtual ~ObjCEventListener();

        virtual void handleEvent(Event*, bool isWindowEvent);

        id <DOMEventListener> m_listener;
    };

} // namespace WebCore

#endif
