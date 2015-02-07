

#ifndef SVGLinearGradientElement_h
#define SVGLinearGradientElement_h

#if ENABLE(SVG)
#include "SVGGradientElement.h"

namespace WebCore {

    struct LinearGradientAttributes;
    class SVGLength;

    class SVGLinearGradientElement : public SVGGradientElement {
    public:
        SVGLinearGradientElement(const QualifiedName&, Document*);
        virtual ~SVGLinearGradientElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

    protected:
        virtual void buildGradient() const;
        virtual SVGPaintServerType gradientType() const { return LinearGradientPaintServer; }

        LinearGradientAttributes collectGradientProperties() const;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGNames::linearGradientTagString, SVGNames::x1AttrString, SVGLength, X1, x1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGNames::linearGradientTagString, SVGNames::y1AttrString, SVGLength, Y1, y1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGNames::linearGradientTagString, SVGNames::x2AttrString, SVGLength, X2, x2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGNames::linearGradientTagString, SVGNames::y2AttrString, SVGLength, Y2, y2)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
