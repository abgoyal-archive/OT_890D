

#ifndef SVGCircleElement_h
#define SVGCircleElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

    class SVGCircleElement : public SVGStyledTransformableElement,
                             public SVGTests,
                             public SVGLangSpace,
                             public SVGExternalResourcesRequired {
    public:
        SVGCircleElement(const QualifiedName&, Document*);
        virtual ~SVGCircleElement();

        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

        virtual Path toPathData() const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }
        virtual bool hasRelativeValues() const;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGCircleElement, SVGNames::circleTagString, SVGNames::cxAttrString, SVGLength, Cx, cx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGCircleElement, SVGNames::circleTagString, SVGNames::cyAttrString, SVGLength, Cy, cy)
        ANIMATED_PROPERTY_DECLARATIONS(SVGCircleElement, SVGNames::circleTagString, SVGNames::rAttrString, SVGLength, R, r)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGCircleElement_h
