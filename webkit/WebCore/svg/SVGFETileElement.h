

#ifndef SVGFETileElement_h
#define SVGFETileElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGFETile.h"

namespace WebCore {

    class SVGFETileElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFETileElement(const QualifiedName&, Document*);
        virtual ~SVGFETileElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETileElement, SVGNames::feTileTagString, SVGNames::inAttrString, String, In1, in1)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
