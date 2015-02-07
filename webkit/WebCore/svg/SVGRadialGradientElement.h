

#ifndef SVGRadialGradientElement_h
#define SVGRadialGradientElement_h

#if ENABLE(SVG)
#include "SVGGradientElement.h"

namespace WebCore {

    struct RadialGradientAttributes;
    class SVGLength;

    class SVGRadialGradientElement : public SVGGradientElement {
    public:
        SVGRadialGradientElement(const QualifiedName&, Document*);
        virtual ~SVGRadialGradientElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

    protected:
        virtual void buildGradient() const;
        virtual SVGPaintServerType gradientType() const { return RadialGradientPaintServer; }

        RadialGradientAttributes collectGradientProperties() const;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGRadialGradientElement, SVGNames::radialGradientTagString, SVGNames::cxAttrString, SVGLength, Cx, cx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGRadialGradientElement, SVGNames::radialGradientTagString, SVGNames::cyAttrString, SVGLength, Cy, cy)
        ANIMATED_PROPERTY_DECLARATIONS(SVGRadialGradientElement, SVGNames::radialGradientTagString, SVGNames::rAttrString, SVGLength, R, r)
        ANIMATED_PROPERTY_DECLARATIONS(SVGRadialGradientElement, SVGNames::radialGradientTagString, SVGNames::fxAttrString, SVGLength, Fx, fx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGRadialGradientElement, SVGNames::radialGradientTagString, SVGNames::fyAttrString, SVGLength, Fy, fy)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
