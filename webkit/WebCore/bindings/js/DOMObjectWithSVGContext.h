

#ifndef DOMObjectWithSVGContext_h
#define DOMObjectWithSVGContext_h

#if ENABLE(SVG)

#include "JSDOMBinding.h"
#include "SVGElement.h"

namespace WebCore {

    // FIXME: This class (and file) should be removed once all SVG bindings
    // have moved context() onto the various impl() pointers.
    class DOMObjectWithSVGContext : public DOMObject {
    public:
        SVGElement* context() const { return m_context.get(); }

    protected:
        DOMObjectWithSVGContext(PassRefPtr<JSC::Structure> structure, JSDOMGlobalObject*, SVGElement* context)
            : DOMObject(structure)
            , m_context(context)
        {
            // No space to store the JSDOMGlobalObject w/o hitting the CELL_SIZE limit.
        }

    protected: // FIXME: Many custom bindings use m_context directly.  Making this protected to temporariliy reduce code churn.
        RefPtr<SVGElement> m_context;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // DOMObjectWithSVGContext_h
