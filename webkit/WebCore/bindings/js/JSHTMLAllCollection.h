

#ifndef JSHTMLAllCollection_h
#define JSHTMLAllCollection_h

#include "HTMLCollection.h"
#include "JSHTMLCollection.h"

namespace WebCore {

    class HTMLCollection;

    class JSHTMLAllCollection : public JSHTMLCollection {
    public:
        JSHTMLAllCollection(PassRefPtr<JSC::Structure> structure, JSDOMGlobalObject* globalObject, PassRefPtr<HTMLCollection> collection)
            : JSHTMLCollection(structure, globalObject, collection)
        {
        }

        static PassRefPtr<JSC::Structure> createStructure(JSC::JSValue proto) 
        { 
            return JSC::Structure::create(proto, JSC::TypeInfo(JSC::ObjectType, JSC::MasqueradesAsUndefined)); 
        }

        static const JSC::ClassInfo s_info;

    private:
        virtual bool toBoolean(JSC::ExecState*) const { return false; }
    };

} // namespace WebCore

#endif // JSHTMLAllCollection_h
