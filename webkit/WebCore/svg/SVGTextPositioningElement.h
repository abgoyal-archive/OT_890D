

#ifndef SVGTextPositioningElement_h
#define SVGTextPositioningElement_h

#if ENABLE(SVG)
#include "SVGTextContentElement.h"
#include "SVGLengthList.h"
#include "SVGNumberList.h"

namespace WebCore {

    extern char SVGTextPositioningElementIdentifier[];

    class SVGTextPositioningElement : public SVGTextContentElement {
    public:
        SVGTextPositioningElement(const QualifiedName&, Document*);
        virtual ~SVGTextPositioningElement();

        virtual void parseMappedAttribute(MappedAttribute*);

        bool isKnownAttribute(const QualifiedName&);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextPositioningElement, SVGTextPositioningElementIdentifier, SVGNames::xAttrString, SVGLengthList, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextPositioningElement, SVGTextPositioningElementIdentifier, SVGNames::yAttrString, SVGLengthList, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextPositioningElement, SVGTextPositioningElementIdentifier, SVGNames::dxAttrString, SVGLengthList, Dx, dx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextPositioningElement, SVGTextPositioningElementIdentifier, SVGNames::dyAttrString, SVGLengthList, Dy, dy)
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextPositioningElement, SVGTextPositioningElementIdentifier, SVGNames::rotateAttrString, SVGNumberList, Rotate, rotate)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
