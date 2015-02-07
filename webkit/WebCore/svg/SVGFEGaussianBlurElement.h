

#ifndef SVGFEGaussianBlurElement_h
#define SVGFEGaussianBlurElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEGaussianBlur.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

    extern char SVGStdDeviationXAttrIdentifier[];
    extern char SVGStdDeviationYAttrIdentifier[];

    class SVGFEGaussianBlurElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEGaussianBlurElement(const QualifiedName&, Document*);
        virtual ~SVGFEGaussianBlurElement();

        void setStdDeviation(float stdDeviationX, float stdDeviationY);

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEGaussianBlurElement, SVGNames::feGaussianBlurTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEGaussianBlurElement, SVGNames::feGaussianBlurTagString, SVGStdDeviationXAttrIdentifier, float, StdDeviationX, stdDeviationX)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEGaussianBlurElement, SVGNames::feGaussianBlurTagString, SVGStdDeviationYAttrIdentifier, float, StdDeviationY, stdDeviationY)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
