

#ifndef JSWrapperObject_h
#define JSWrapperObject_h

#include "JSObject.h"

namespace JSC {
    
    // This class is used as a base for classes such as String,
    // Number, Boolean and Date which are wrappers for primitive types.
    class JSWrapperObject : public JSObject {
    protected:
        explicit JSWrapperObject(PassRefPtr<Structure>);

    public:
        JSValue internalValue() const { return m_internalValue; }
        void setInternalValue(JSValue);
        
        virtual void markChildren(MarkStack&);
        
    private:
        JSValue m_internalValue;
    };
    
    inline JSWrapperObject::JSWrapperObject(PassRefPtr<Structure> structure)
        : JSObject(structure)
    {
    }
    
    inline void JSWrapperObject::setInternalValue(JSValue value)
    {
        ASSERT(value);
        ASSERT(!value.isObject());
        m_internalValue = value;
    }

} // namespace JSC

#endif // JSWrapperObject_h
