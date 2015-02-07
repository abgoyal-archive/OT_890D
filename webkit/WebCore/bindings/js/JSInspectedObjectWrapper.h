

#ifndef JSInspectedObjectWrapper_h
#define JSInspectedObjectWrapper_h

#include "JSQuarantinedObjectWrapper.h"

namespace WebCore {

    class JSInspectedObjectWrapper : public JSQuarantinedObjectWrapper {
    public:
        static JSC::JSValue wrap(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue);
        virtual ~JSInspectedObjectWrapper();

        static const JSC::ClassInfo s_info;

    private:
        JSInspectedObjectWrapper(JSC::ExecState* unwrappedExec, JSC::JSObject* unwrappedObject, PassRefPtr<JSC::Structure>);

        virtual bool allowsGetProperty() const { return true; }
        virtual bool allowsSetProperty() const { return true; }
        virtual bool allowsDeleteProperty() const { return true; }
        virtual bool allowsConstruct() const { return true; }
        virtual bool allowsHasInstance() const { return true; }
        virtual bool allowsCallAsFunction() const { return true; }
        virtual bool allowsGetPropertyNames() const { return true; }

        virtual JSC::JSValue prepareIncomingValue(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue) const;
        virtual JSC::JSValue wrapOutgoingValue(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue) const { return wrap(unwrappedExec, unwrappedValue); }

        virtual const JSC::ClassInfo* classInfo() const { return &s_info; }
    };

} // namespace WebCore

#endif // JSInspectedObjectWrapper_h
