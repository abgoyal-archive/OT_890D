

#ifndef SVGFEBlendElement_h
#define SVGFEBlendElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEBlend.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    class SVGFEBlendElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEBlendElement(const QualifiedName&, Document*);
        virtual ~SVGFEBlendElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEBlendElement, SVGNames::feBlendTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEBlendElement, SVGNames::feBlendTagString, SVGNames::in2AttrString, String, In2, in2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEBlendElement, SVGNames::feBlendTagString, SVGNames::modeAttrString, int, Mode, mode)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
