

#ifndef SVGFEFloodElement_h
#define SVGFEFloodElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEFlood.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    class SVGFEFloodElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEFloodElement(const QualifiedName&, Document*);
        virtual ~SVGFEFloodElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEFloodElement, SVGNames::feFloodTagString, SVGNames::inAttrString, String, In1, in1)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
