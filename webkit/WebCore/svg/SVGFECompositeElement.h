

#ifndef SVGFECompositeElement_h
#define SVGFECompositeElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEComposite.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

    class SVGFECompositeElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFECompositeElement(const QualifiedName&, Document*);
        virtual ~SVGFECompositeElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::in2AttrString, String, In2, in2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::operatorAttrString, int, _operator, _operator)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::k1AttrString, float, K1, k1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::k2AttrString, float, K2, k2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::k3AttrString, float, K3, k3)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFECompositeElement, SVGNames::feCompositeTagString, SVGNames::k4AttrString, float, K4, k4)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
