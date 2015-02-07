

#ifndef JSCustomVoidCallback_h
#define JSCustomVoidCallback_h

#include "VoidCallback.h"

#include <runtime/JSObject.h>
#include <runtime/Protect.h>
#include <wtf/Forward.h>

namespace WebCore {
    
    class Frame;
    
    class JSCustomVoidCallback : public VoidCallback {
    public: 
        static PassRefPtr<JSCustomVoidCallback> create(JSC::JSObject* callback, Frame* frame)
        {
            return adoptRef(new JSCustomVoidCallback(callback, frame));
        }
        
        virtual void handleEvent();
        
    private:
        JSCustomVoidCallback(JSC::JSObject* callback, Frame*);

        JSC::ProtectedPtr<JSC::JSObject> m_callback;
        RefPtr<Frame> m_frame;
    };

    PassRefPtr<VoidCallback> toVoidCallback(JSC::ExecState*, JSC::JSValue);

} // namespace WebCore

#endif // JSCustomVoidCallback_h
