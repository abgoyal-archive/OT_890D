

#ifndef JSPropertyNameIterator_h
#define JSPropertyNameIterator_h

#include "JSObject.h"
#include "JSString.h"
#include "PropertyNameArray.h"

namespace JSC {

    class Identifier;
    class JSObject;

    class JSPropertyNameIterator : public JSCell {
    public:
        static JSPropertyNameIterator* create(ExecState*, JSValue);

        virtual ~JSPropertyNameIterator();

        virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        virtual bool getPrimitiveNumber(ExecState*, double&, JSValue&);
        virtual bool toBoolean(ExecState*) const;
        virtual double toNumber(ExecState*) const;
        virtual UString toString(ExecState*) const;
        virtual JSObject* toObject(ExecState*) const;

        virtual void markChildren(MarkStack&);

        JSValue next(ExecState*);
        void invalidate();
        
        static PassRefPtr<Structure> createStructure(JSValue prototype)
        {
            return Structure::create(prototype, TypeInfo(CompoundType));
        }
    private:
        JSPropertyNameIterator(ExecState*);
        JSPropertyNameIterator(ExecState*, JSObject*, PassRefPtr<PropertyNameArrayData> propertyNameArrayData);

        JSObject* m_object;
        RefPtr<PropertyNameArrayData> m_data;
        PropertyNameArrayData::const_iterator m_position;
        PropertyNameArrayData::const_iterator m_end;
    };

inline JSPropertyNameIterator::JSPropertyNameIterator(ExecState* exec)
    : JSCell(exec->globalData().propertyNameIteratorStructure.get())
    , m_object(0)
    , m_position(0)
    , m_end(0)
{
}

inline JSPropertyNameIterator::JSPropertyNameIterator(ExecState* exec, JSObject* object, PassRefPtr<PropertyNameArrayData> propertyNameArrayData)
    : JSCell(exec->globalData().propertyNameIteratorStructure.get())
    , m_object(object)
    , m_data(propertyNameArrayData)
    , m_position(m_data->begin())
    , m_end(m_data->end())
{
}

inline JSPropertyNameIterator* JSPropertyNameIterator::create(ExecState* exec, JSValue v)
{
    if (v.isUndefinedOrNull())
        return new (exec) JSPropertyNameIterator(exec);

    JSObject* o = v.toObject(exec);
    PropertyNameArray propertyNames(exec);
    o->getPropertyNames(exec, propertyNames);
    return new (exec) JSPropertyNameIterator(exec, o, propertyNames.releaseData());
}

inline JSValue JSPropertyNameIterator::next(ExecState* exec)
{
    if (m_position == m_end)
        return JSValue();

    if (m_data->cachedStructure() == m_object->structure() && m_data->cachedPrototypeChain() == m_object->structure()->prototypeChain(exec))
        return jsOwnedString(exec, (*m_position++).ustring());

    do {
        if (m_object->hasProperty(exec, *m_position))
            return jsOwnedString(exec, (*m_position++).ustring());
        m_position++;
    } while (m_position != m_end);

    return JSValue();
}

} // namespace JSC

#endif // JSPropertyNameIterator_h
