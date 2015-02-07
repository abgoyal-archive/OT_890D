

#ifndef JSInspectorCallbackWrapper_h
#define JSInspectorCallbackWrapper_h

#include "JSQuarantinedObjectWrapper.h"

namespace WebCore {

    class JSInspectorCallbackWrapper : public JSQuarantinedObjectWrapper {
    public:
        static JSC::JSValue wrap(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue);

        virtual ~JSInspectorCallbackWrapper();

        virtual const JSC::ClassInfo* classInfo() const { return &s_info; }
        static const JSC::ClassInfo s_info;

    protected:
        JSInspectorCallbackWrapper(JSC::ExecState* unwrappedExec, JSC::JSObject* unwrappedObject, PassRefPtr<JSC::Structure>);

        virtual bool allowsCallAsFunction() const { return true; }

        virtual JSC::JSValue prepareIncomingValue(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue) const;
        virtual JSC::JSValue wrapOutgoingValue(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue) const { return wrap(unwrappedExec, unwrappedValue); }
    };

} // namespace WebCore

#endif // JSInspectorCallbackWrapper_h
