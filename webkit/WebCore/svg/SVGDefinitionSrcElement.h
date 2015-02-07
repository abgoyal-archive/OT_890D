

#ifndef SVGDefinitionSrcElement_h
#define SVGDefinitionSrcElement_h

#if ENABLE(SVG_FONTS)
#include "SVGElement.h"

namespace WebCore {
    class SVGDefinitionSrcElement : public SVGElement {
    public:
        SVGDefinitionSrcElement(const QualifiedName&, Document*);
    
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);
    };

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif

// vim:ts=4:noet
