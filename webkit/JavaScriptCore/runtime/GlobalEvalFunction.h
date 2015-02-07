

#ifndef GlobalEvalFunction_h
#define GlobalEvalFunction_h

#include "PrototypeFunction.h"

namespace JSC {

    class JSGlobalObject;

    class GlobalEvalFunction : public PrototypeFunction {
    public:
        GlobalEvalFunction(ExecState*, PassRefPtr<Structure>, int len, const Identifier&, NativeFunction, JSGlobalObject* expectedThisObject);
        JSGlobalObject* cachedGlobalObject() const { return m_cachedGlobalObject; }

    private:
        virtual void markChildren(MarkStack&);

        JSGlobalObject* m_cachedGlobalObject;
    };

} // namespace JSC

#endif // GlobalEvalFunction_h
