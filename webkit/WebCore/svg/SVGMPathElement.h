

#ifndef SVGMPathElement_h
#define SVGMPathElement_h
#if ENABLE(SVG)

#include "SVGURIReference.h"
#include "SVGExternalResourcesRequired.h"

namespace WebCore {
    
    class SVGPathElement;
    
    class SVGMPathElement : public SVGElement,
                                   SVGURIReference,
                                   SVGExternalResourcesRequired {
    public:
        SVGMPathElement(const QualifiedName&, Document*);
        virtual ~SVGMPathElement();
        
        virtual void parseMappedAttribute(MappedAttribute*);
        
        SVGPathElement* pathElement();
        
    protected:
        virtual const SVGElement* contextElement() const { return this; }
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGMPathElement_h

// vim:ts=4:noet
