

#ifndef KJS_RUNTIME_OBJECT_H
#define KJS_RUNTIME_OBJECT_H

#include "runtime.h"
#include <runtime/JSGlobalObject.h>

namespace JSC {

class RuntimeObjectImp : public JSObject {
public:
    RuntimeObjectImp(ExecState*, PassRefPtr<Bindings::Instance>);

    virtual ~RuntimeObjectImp();

    virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
    virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);
    virtual bool deleteProperty(ExecState* , const Identifier& propertyName);
    virtual JSValue defaultValue(ExecState*, PreferredPrimitiveType) const;
    virtual CallType getCallData(CallData&);
    virtual ConstructType getConstructData(ConstructData&);
    
    virtual void getPropertyNames(ExecState*, PropertyNameArray&);

    virtual void invalidate();
    Bindings::Instance* getInternalInstance() const { return instance.get(); }

    static JSObject* throwInvalidAccessError(ExecState*);

    static const ClassInfo s_info;

    static ObjectPrototype* createPrototype(ExecState*, JSGlobalObject* globalObject)
    {
        return globalObject->objectPrototype();
    }

    static PassRefPtr<Structure> createStructure(JSValue prototype)
    {
        return Structure::create(prototype, TypeInfo(ObjectType));
    }

protected:
    RuntimeObjectImp(ExecState*, PassRefPtr<Structure>, PassRefPtr<Bindings::Instance>);

private:
    virtual const ClassInfo* classInfo() const { return &s_info; }
    
    static JSValue fallbackObjectGetter(ExecState*, const Identifier&, const PropertySlot&);
    static JSValue fieldGetter(ExecState*, const Identifier&, const PropertySlot&);
    static JSValue methodGetter(ExecState*, const Identifier&, const PropertySlot&);

    RefPtr<Bindings::Instance> instance;
};
    
} // namespace

#endif
