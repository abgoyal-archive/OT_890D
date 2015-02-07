

#ifndef SVGFilterPrimitiveStandardAttributes_h
#define SVGFilterPrimitiveStandardAttributes_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFilterBuilder.h"
#include "SVGResourceFilter.h"
#include "SVGStyledElement.h"

namespace WebCore {

    extern char SVGFilterPrimitiveStandardAttributesIdentifier[];

    class SVGResourceFilter;

    class SVGFilterPrimitiveStandardAttributes : public SVGStyledElement {
    public:
        SVGFilterPrimitiveStandardAttributes(const QualifiedName&, Document*);
        virtual ~SVGFilterPrimitiveStandardAttributes();
        
        virtual bool isFilterEffect() const { return true; }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*) = 0;

        virtual bool rendererIsNeeded(RenderStyle*) { return false; }

    protected:
        friend class SVGResourceFilter;
        void setStandardAttributes(SVGResourceFilter*, FilterEffect*) const;
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFilterPrimitiveStandardAttributes, SVGFilterPrimitiveStandardAttributesIdentifier, SVGNames::xAttrString, SVGLength, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFilterPrimitiveStandardAttributes, SVGFilterPrimitiveStandardAttributesIdentifier, SVGNames::yAttrString, SVGLength, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFilterPrimitiveStandardAttributes, SVGFilterPrimitiveStandardAttributesIdentifier, SVGNames::widthAttrString, SVGLength, Width, width)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFilterPrimitiveStandardAttributes, SVGFilterPrimitiveStandardAttributesIdentifier, SVGNames::heightAttrString, SVGLength, Height, height)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFilterPrimitiveStandardAttributes, SVGFilterPrimitiveStandardAttributesIdentifier, SVGNames::resultAttrString, String, Result, result)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
