

#ifndef JSAPIValueWrapper_h
#define JSAPIValueWrapper_h

#include <wtf/Platform.h>

#include "JSCell.h"
#include "CallFrame.h"

namespace JSC {

    class JSAPIValueWrapper : public JSCell {
        friend JSValue jsAPIValueWrapper(ExecState*, JSValue);
    public:
        JSValue value() const { return m_value; }

        virtual bool isAPIValueWrapper() const { return true; }

        virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(ExecState*, double& number, JSValue&);
        virtual bool toBoolean(ExecState*) const;
        virtual double toNumber(ExecState*) const;
        virtual UString toString(ExecState*) const;
        virtual JSObject* toObject(ExecState*) const;
        static PassRefPtr<Structure> createStructure(JSValue prototype)
        {
            return Structure::create(prototype, TypeInfo(CompoundType));
        }

        
    private:
        JSAPIValueWrapper(ExecState* exec, JSValue value)
            : JSCell(exec->globalData().apiWrapperStructure.get())
            , m_value(value)
        {
        }

        JSValue m_value;
    };

    inline JSValue jsAPIValueWrapper(ExecState* exec, JSValue value)
    {
        return new (exec) JSAPIValueWrapper(exec, value);
    }

} // namespace JSC

#endif // JSAPIValueWrapper_h
