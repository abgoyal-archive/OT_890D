

#ifndef SVGFEColorMatrixElement_h
#define SVGFEColorMatrixElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEColorMatrix.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGNumberList.h"

namespace WebCore {

    class SVGFEColorMatrixElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEColorMatrixElement(const QualifiedName&, Document*);
        virtual ~SVGFEColorMatrixElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEColorMatrixElement, SVGNames::feColorMatrixTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEColorMatrixElement, SVGNames::feColorMatrixTagString, SVGNames::typeAttrString, int, Type, type)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEColorMatrixElement, SVGNames::feColorMatrixTagString, SVGNames::valuesAttrString, SVGNumberList, Values, values)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
