

#ifndef JSCustomSQLStatementCallback_h
#define JSCustomSQLStatementCallback_h

#if ENABLE(DATABASE)

#include "SQLStatementCallback.h"

#include <runtime/JSObject.h>
#include <runtime/Protect.h>
#include <wtf/Forward.h>

namespace JSC {
    class JSObject;
}

namespace WebCore {

class Frame;
class SQLResultSet;

class JSCustomSQLStatementCallback : public SQLStatementCallback {
public:
    static PassRefPtr<JSCustomSQLStatementCallback> create(JSC::JSObject* callback, Frame* frame) { return adoptRef(new JSCustomSQLStatementCallback(callback, frame)); }

    virtual void handleEvent(SQLTransaction*, SQLResultSet*, bool& raisedException);

private:
    JSCustomSQLStatementCallback(JSC::JSObject* callback, Frame*);

    JSC::ProtectedPtr<JSC::JSObject> m_callback;
    RefPtr<Frame> m_frame;
};

}

#endif // ENABLE(DATABASE)

#endif // JSCustomSQLStatementCallback_h
