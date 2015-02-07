

#ifndef JSEventListener_h
#define JSEventListener_h

#include "EventListener.h"
#include "JSDOMWindow.h"
#include <runtime/Protect.h>

namespace WebCore {

    class JSDOMGlobalObject;

    class JSEventListener : public EventListener {
    public:
        static PassRefPtr<JSEventListener> create(JSC::JSObject* listener, JSDOMGlobalObject* globalObject, bool isAttribute)
        {
            return adoptRef(new JSEventListener(listener, globalObject, isAttribute));
        }
        virtual ~JSEventListener();
        void clearGlobalObject() { m_globalObject = 0; }

        // Returns true if this event listener was created for an event handler attribute, like "onload" or "onclick".
        bool isAttribute() const { return m_isAttribute; }

        virtual JSC::JSObject* jsFunction() const;

    private:
        virtual void markJSFunction(JSC::MarkStack&);
        virtual void handleEvent(Event*, bool isWindowEvent);
        virtual bool reportError(const String& message, const String& url, int lineNumber);
        virtual bool virtualisAttribute() const;
        void clearJSFunctionInline();

    protected:
        JSEventListener(JSC::JSObject* function, JSDOMGlobalObject*, bool isAttribute);

        mutable JSC::JSObject* m_jsFunction;
        JSDOMGlobalObject* m_globalObject;
        bool m_isAttribute;
    };

} // namespace WebCore

#endif // JSEventListener_h
