

#ifndef JSNamedNodesCollection_h
#define JSNamedNodesCollection_h

#include "JSDOMBinding.h"
#include <wtf/Vector.h>

namespace WebCore {

    class Node;

    // Internal class, used for the collection return by e.g. document.forms.myinput
    // when multiple nodes have the same name.
    class JSNamedNodesCollection : public DOMObjectWithGlobalPointer {
    public:
        JSNamedNodesCollection(JSC::ExecState*, JSDOMGlobalObject*, const Vector<RefPtr<Node> >&);

        virtual bool getOwnPropertySlot(JSC::ExecState*, const JSC::Identifier&, JSC::PropertySlot&);

        virtual const JSC::ClassInfo* classInfo() const { return &s_info; }
        static const JSC::ClassInfo s_info;

        static JSC::ObjectPrototype* createPrototype(JSC::ExecState*, JSC::JSGlobalObject* globalObject)
        {
            return globalObject->objectPrototype();
        }

        static PassRefPtr<JSC::Structure> createStructure(JSC::JSValue prototype)
        {
            return JSC::Structure::create(prototype, JSC::TypeInfo(JSC::ObjectType));
        }

    private:
        static JSC::JSValue lengthGetter(JSC::ExecState*, const JSC::Identifier&, const JSC::PropertySlot&);
        static JSC::JSValue indexGetter(JSC::ExecState*, const JSC::Identifier&, const JSC::PropertySlot&);

        OwnPtr<Vector<RefPtr<Node> > > m_nodes;
    };

} // namespace WebCore

#endif // JSNamedNodesCollection_h
