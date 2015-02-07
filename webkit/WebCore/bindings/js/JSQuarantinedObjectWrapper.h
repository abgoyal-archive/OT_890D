

#ifndef JSQuarantinedObjectWrapper_h
#define JSQuarantinedObjectWrapper_h

#include <runtime/JSObject.h>

namespace WebCore {

    class JSQuarantinedObjectWrapper : public JSC::JSObject {
    public:
        static JSQuarantinedObjectWrapper* asWrapper(JSC::JSValue);

        virtual ~JSQuarantinedObjectWrapper();

        virtual JSC::JSObject* unwrappedObject() { return m_unwrappedObject; }

        JSC::JSGlobalObject* unwrappedGlobalObject() const { return m_unwrappedGlobalObject; };
        JSC::ExecState* unwrappedExecState() const;

        bool allowsUnwrappedAccessFrom(JSC::ExecState*) const;

        static const JSC::ClassInfo s_info;

        static PassRefPtr<JSC::Structure> createStructure(JSC::JSValue proto) 
        { 
            return JSC::Structure::create(proto, JSC::TypeInfo(JSC::ObjectType, JSC::ImplementsHasInstance | JSC::OverridesHasInstance)); 
        }

    protected:
        JSQuarantinedObjectWrapper(JSC::ExecState* unwrappedExec, JSC::JSObject* unwrappedObject, PassRefPtr<JSC::Structure>);

        virtual void markChildren(JSC::MarkStack&);

    private:
        virtual bool getOwnPropertySlot(JSC::ExecState*, const JSC::Identifier&, JSC::PropertySlot&);
        virtual bool getOwnPropertySlot(JSC::ExecState*, unsigned, JSC::PropertySlot&);

        virtual void put(JSC::ExecState*, const JSC::Identifier&, JSC::JSValue, JSC::PutPropertySlot&);
        virtual void put(JSC::ExecState*, unsigned, JSC::JSValue);

        virtual bool deleteProperty(JSC::ExecState*, const JSC::Identifier&);
        virtual bool deleteProperty(JSC::ExecState*, unsigned);

        virtual JSC::CallType getCallData(JSC::CallData&);
        virtual JSC::ConstructType getConstructData(JSC::ConstructData&);

        virtual bool hasInstance(JSC::ExecState*, JSC::JSValue, JSC::JSValue proto);

        virtual void getPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&);

        virtual JSC::UString className() const { return m_unwrappedObject->className(); }

        virtual bool allowsGetProperty() const { return false; }
        virtual bool allowsSetProperty() const { return false; }
        virtual bool allowsDeleteProperty() const { return false; }
        virtual bool allowsConstruct() const { return false; }
        virtual bool allowsHasInstance() const { return false; }
        virtual bool allowsCallAsFunction() const { return false; }
        virtual bool allowsGetPropertyNames() const { return false; }

        virtual JSC::JSValue prepareIncomingValue(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue) const = 0;
        virtual JSC::JSValue wrapOutgoingValue(JSC::ExecState* unwrappedExec, JSC::JSValue unwrappedValue) const = 0;

        static JSC::JSValue cachedValueGetter(JSC::ExecState*, const JSC::Identifier&, const JSC::PropertySlot&);

        void transferExceptionToExecState(JSC::ExecState*) const;

        static JSC::JSValue JSC_HOST_CALL call(JSC::ExecState*, JSC::JSObject* function, JSC::JSValue thisValue, const JSC::ArgList&);
        static JSC::JSObject* construct(JSC::ExecState*, JSC::JSObject*, const JSC::ArgList&);

        JSC::JSGlobalObject* m_unwrappedGlobalObject;
        JSC::JSObject* m_unwrappedObject;
    };

} // namespace WebCore

#endif // JSQuarantinedObjectWrapper_h
