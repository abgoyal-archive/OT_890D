

#ifndef SVGEllipseElement_h
#define SVGEllipseElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

    class SVGEllipseElement : public SVGStyledTransformableElement,
                              public SVGTests,
                              public SVGLangSpace,
                              public SVGExternalResourcesRequired {
    public:
        SVGEllipseElement(const QualifiedName&, Document*);
        virtual ~SVGEllipseElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

        virtual Path toPathData() const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }
        virtual bool hasRelativeValues() const;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGEllipseElement, SVGNames::ellipseTagString, SVGNames::cxAttrString, SVGLength, Cx, cx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGEllipseElement, SVGNames::ellipseTagString, SVGNames::cyAttrString, SVGLength, Cy, cy)
        ANIMATED_PROPERTY_DECLARATIONS(SVGEllipseElement, SVGNames::ellipseTagString, SVGNames::rxAttrString, SVGLength, Rx, rx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGEllipseElement, SVGNames::ellipseTagString, SVGNames::ryAttrString, SVGLength, Ry, ry)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
