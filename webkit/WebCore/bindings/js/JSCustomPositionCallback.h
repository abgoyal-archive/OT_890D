

#ifndef JSCustomPositionCallback_h
#define JSCustomPositionCallback_h

#include "PositionCallback.h"
#include <runtime/JSObject.h>
#include <runtime/Protect.h>
#include <wtf/Forward.h>

namespace JSC {
    class JSObject;
}

namespace WebCore {

class Frame;
class Geoposition;

class JSCustomPositionCallback : public PositionCallback {
public:
    static PassRefPtr<JSCustomPositionCallback> create(JSC::JSObject* callback, Frame* frame) { return adoptRef(new JSCustomPositionCallback(callback, frame)); }
    
    virtual void handleEvent(Geoposition*);

private:
    JSCustomPositionCallback(JSC::JSObject* callback, Frame*);

    JSC::ProtectedPtr<JSC::JSObject> m_callback;
    RefPtr<Frame> m_frame;
};
    
} // namespace WebCore

#endif // JSCustomPositionCallback_h
