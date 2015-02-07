

#ifndef SVGFEOffsetElement_h
#define SVGFEOffsetElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGFEOffset.h"

namespace WebCore {

    class SVGFEOffsetElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEOffsetElement(const QualifiedName&, Document*);
        virtual ~SVGFEOffsetElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEOffsetElement, SVGNames::feOffsetTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEOffsetElement, SVGNames::feOffsetTagString, SVGNames::dxAttrString, float, Dx, dx)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEOffsetElement, SVGNames::feOffsetTagString, SVGNames::dyAttrString, float, Dy, dy)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
