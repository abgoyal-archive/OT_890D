

#ifndef SVGFEMergeNodeElement_h
#define SVGFEMergeNodeElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGElement.h"

namespace WebCore {

    class SVGFEMergeNodeElement : public SVGElement {
    public:
        SVGFEMergeNodeElement(const QualifiedName&, Document*);
        virtual ~SVGFEMergeNodeElement();

        virtual void parseMappedAttribute(MappedAttribute*);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEMergeNodeElement, SVGNames::feMergeNodeTagString, SVGNames::inAttrString, String, In1, in1)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
