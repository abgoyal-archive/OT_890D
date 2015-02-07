

#ifndef JSLazyEventListener_h
#define JSLazyEventListener_h

#include "JSEventListener.h"
#include "PlatformString.h"

namespace WebCore {

    class Node;

    class JSLazyEventListener : public JSEventListener {
    public:
        static PassRefPtr<JSLazyEventListener> create(const String& functionName, const String& eventParameterName, const String& code, JSDOMGlobalObject* globalObject, Node* node, int lineNumber)
        {
            return adoptRef(new JSLazyEventListener(functionName, eventParameterName, code, globalObject, node, lineNumber));
        }
        virtual ~JSLazyEventListener();

    private:
        JSLazyEventListener(const String& functionName, const String& eventParameterName, const String& code, JSDOMGlobalObject*, Node*, int lineNumber);

        virtual JSC::JSObject* jsFunction() const;
        virtual bool wasCreatedFromMarkup() const { return true; }

        void parseCode() const;

        mutable String m_functionName;
        mutable String m_eventParameterName;
        mutable String m_code;
        mutable bool m_parsed;
        int m_lineNumber;
        Node* m_originalNode;
    };

} // namespace WebCore

#endif // JSEventListener_h
