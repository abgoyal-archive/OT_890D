

#ifndef JSCustomSQLTransactionErrorCallback_h
#define JSCustomSQLTransactionErrorCallback_h

#if ENABLE(DATABASE)

#include "SQLTransactionErrorCallback.h"

#include <runtime/JSObject.h>
#include <runtime/Protect.h>
#include <wtf/Forward.h>

namespace JSC {
    class JSObject;
}

namespace WebCore {

class Frame;
class SQLError;

class JSCustomSQLTransactionErrorCallback : public SQLTransactionErrorCallback {
public:
    static PassRefPtr<JSCustomSQLTransactionErrorCallback> create(JSC::JSObject* callback, Frame* frame) { return adoptRef(new JSCustomSQLTransactionErrorCallback(callback, frame)); }
    
    virtual void handleEvent(SQLError*);

private:
    JSCustomSQLTransactionErrorCallback(JSC::JSObject* callback, Frame*);

    JSC::ProtectedPtr<JSC::JSObject> m_callback;
    RefPtr<Frame> m_frame;
};

}
#endif // ENABLE(DATABASE)

#endif // JSCustomSQLTransactionErrorCallback_h
