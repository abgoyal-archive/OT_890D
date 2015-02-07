

#ifndef JSCustomPositionErrorCallback_h
#define JSCustomPositionErrorCallback_h

#include "PositionErrorCallback.h"
#include <runtime/JSObject.h>
#include <runtime/Protect.h>
#include <wtf/Forward.h>

namespace JSC {
    class JSObject;
}

namespace WebCore {
    
class Frame;
class PositionError;

class JSCustomPositionErrorCallback : public PositionErrorCallback {
public:
    static PassRefPtr<JSCustomPositionErrorCallback> create(JSC::JSObject* callback, Frame* frame) { return adoptRef(new JSCustomPositionErrorCallback(callback, frame)); }
    
    virtual void handleEvent(PositionError*);

private:
    JSCustomPositionErrorCallback(JSC::JSObject* callback, Frame*);

    JSC::ProtectedPtr<JSC::JSObject> m_callback;
    RefPtr<Frame> m_frame;
};
    
} // namespace WebCore

#endif // JSCustomPositionErrorCallback_h
