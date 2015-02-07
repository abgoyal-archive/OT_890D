

#ifndef SVGLineElement_h
#define SVGLineElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

    class SVGLength;

    class SVGLineElement : public SVGStyledTransformableElement,
                           public SVGTests,
                           public SVGLangSpace,
                           public SVGExternalResourcesRequired {
    public:
        SVGLineElement(const QualifiedName&, Document*);
        virtual ~SVGLineElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

        virtual Path toPathData() const;

        virtual bool supportsMarkers() const { return true; }

    protected:
        virtual const SVGElement* contextElement() const { return this; }
        virtual bool hasRelativeValues() const;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGLineElement, SVGNames::lineTagString, SVGNames::x1AttrString, SVGLength, X1, x1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLineElement, SVGNames::lineTagString, SVGNames::y1AttrString, SVGLength, Y1, y1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLineElement, SVGNames::lineTagString, SVGNames::x2AttrString, SVGLength, X2, x2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLineElement, SVGNames::lineTagString, SVGNames::y2AttrString, SVGLength, Y2, y2)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
