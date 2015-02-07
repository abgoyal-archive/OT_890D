

#ifndef JSCustomSQLTransactionCallback_h
#define JSCustomSQLTransactionCallback_h

#if ENABLE(DATABASE)

#include "SQLTransactionCallback.h"

#include <wtf/PassRefPtr.h>

namespace JSC {
    class JSObject;
}

namespace WebCore {

class Frame;

class JSCustomSQLTransactionCallback : public SQLTransactionCallback {
public:
    static PassRefPtr<JSCustomSQLTransactionCallback> create(JSC::JSObject* callback, Frame* frame) { return adoptRef(new JSCustomSQLTransactionCallback(callback, frame)); }

    virtual ~JSCustomSQLTransactionCallback();
    
    virtual void handleEvent(SQLTransaction*, bool& raisedException);

private:
    JSCustomSQLTransactionCallback(JSC::JSObject* callback, Frame*);

    static void deleteData(void*);

    class Data;
    Data* m_data;
};

}

#endif // ENABLE(DATABASE)

#endif // JSCustomSQLTransactionCallback_h
