

#ifndef BINDINGS_C_INSTANCE_H_
#define BINDINGS_C_INSTANCE_H_

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "runtime.h"
#include <wtf/PassRefPtr.h>
#include "runtime_root.h"

typedef struct NPObject NPObject;

namespace JSC {

class UString;

namespace Bindings {

class CClass;

class CInstance : public Instance {
public:
    static PassRefPtr<CInstance> create(NPObject* object, PassRefPtr<RootObject> rootObject)
    {
        return adoptRef(new CInstance(object, rootObject));
    }

    static void setGlobalException(JSC::UString exception);

    ~CInstance ();

    virtual Class *getClass() const;

    virtual JSValue valueOf(ExecState*) const;
    virtual JSValue defaultValue(ExecState*, PreferredPrimitiveType) const;

    virtual JSValue invokeMethod(ExecState*, const MethodList&, const ArgList&);
    virtual bool supportsInvokeDefaultMethod() const;
    virtual JSValue invokeDefaultMethod(ExecState*, const ArgList&);

    virtual bool supportsConstruct() const;
    virtual JSValue invokeConstruct(ExecState*, const ArgList&);

    virtual void getPropertyNames(ExecState*, PropertyNameArray&);

    JSValue stringValue(ExecState*) const;
    JSValue numberValue(ExecState*) const;
    JSValue booleanValue() const;

    NPObject *getObject() const { return _object; }

private:
    static void moveGlobalExceptionToExecState(ExecState* exec);
    CInstance(NPObject*, PassRefPtr<RootObject>);

    mutable CClass *_class;
    NPObject *_object;
};

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif
