

#ifndef JSCustomSQLStatementErrorCallback_h
#define JSCustomSQLStatementErrorCallback_h

#if ENABLE(DATABASE)

#include "SQLStatementErrorCallback.h"

#include <runtime/JSObject.h>
#include <runtime/Protect.h>
#include <wtf/Forward.h>

namespace JSC {
    class JSObject;
}

namespace WebCore {
    
class Frame;
class SQLError;
    
class JSCustomSQLStatementErrorCallback : public SQLStatementErrorCallback {
public:
    static PassRefPtr<JSCustomSQLStatementErrorCallback> create(JSC::JSObject* callback, Frame* frame) { return adoptRef(new JSCustomSQLStatementErrorCallback(callback, frame)); }
        
    virtual bool handleEvent(SQLTransaction*, SQLError*);

private:
    JSCustomSQLStatementErrorCallback(JSC::JSObject* callback, Frame*);

    JSC::ProtectedPtr<JSC::JSObject> m_callback;
    RefPtr<Frame> m_frame;
};
    
}

#endif // ENABLE(DATABASE)

#endif // JSCustomSQLStatementErrorCallback_h

