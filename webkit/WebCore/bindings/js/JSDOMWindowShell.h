

#ifndef JSDOMWindowShell_h
#define JSDOMWindowShell_h

#include "JSDOMWindow.h"
#include "JSDOMBinding.h"

namespace WebCore {

    class DOMWindow;
    class Frame;

    class JSDOMWindowShell : public DOMObject {
        typedef DOMObject Base;
    public:
        JSDOMWindowShell(PassRefPtr<DOMWindow>);
        virtual ~JSDOMWindowShell();

        JSDOMWindow* window() const { return m_window; }
        void setWindow(JSDOMWindow* window)
        {
            ASSERT_ARG(window, window);
            m_window = window;
            setPrototype(window->prototype());
        }
        void setWindow(PassRefPtr<DOMWindow>);

        static const JSC::ClassInfo s_info;

        DOMWindow* impl() const;

        void* operator new(size_t);

        static PassRefPtr<JSC::Structure> createStructure(JSC::JSValue prototype) 
        {
            return JSC::Structure::create(prototype, JSC::TypeInfo(JSC::ObjectType)); 
        }

    private:
        virtual void markChildren(JSC::MarkStack&);
        virtual JSC::UString className() const;
        virtual bool getOwnPropertySlot(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::PropertySlot&);
        virtual void put(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSValue, JSC::PutPropertySlot&);
        virtual void putWithAttributes(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSValue, unsigned attributes);
        virtual bool deleteProperty(JSC::ExecState*, const JSC::Identifier& propertyName);
        virtual void getPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&);
        virtual bool getPropertyAttributes(JSC::ExecState*, const JSC::Identifier& propertyName, unsigned& attributes) const;
        virtual void defineGetter(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSObject* getterFunction);
        virtual void defineSetter(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSObject* setterFunction);
        virtual JSC::JSValue lookupGetter(JSC::ExecState*, const JSC::Identifier& propertyName);
        virtual JSC::JSValue lookupSetter(JSC::ExecState*, const JSC::Identifier& propertyName);
        virtual JSC::JSObject* unwrappedObject();
        virtual const JSC::ClassInfo* classInfo() const { return &s_info; }

        JSDOMWindow* m_window;
    };

    JSC::JSValue toJS(JSC::ExecState*, Frame*);
    JSDOMWindowShell* toJSDOMWindowShell(Frame*);

} // namespace WebCore

#endif // JSDOMWindowShell_h
