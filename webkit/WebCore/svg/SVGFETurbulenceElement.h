

#ifndef SVGFETurbulenceElement_h
#define SVGFETurbulenceElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFETurbulence.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

    extern char SVGBaseFrequencyXIdentifier[];
    extern char SVGBaseFrequencyYIdentifier[];

    enum SVGStitchOptions {
        SVG_STITCHTYPE_UNKNOWN  = 0,
        SVG_STITCHTYPE_STITCH   = 1,
        SVG_STITCHTYPE_NOSTITCH = 2
    };

    class SVGFETurbulenceElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFETurbulenceElement(const QualifiedName&, Document*);
        virtual ~SVGFETurbulenceElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETurbulenceElement, SVGNames::feTurbulenceTagString, SVGBaseFrequencyXIdentifier, float, BaseFrequencyX, baseFrequencyX)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETurbulenceElement, SVGNames::feTurbulenceTagString, SVGBaseFrequencyYIdentifier, float, BaseFrequencyY, baseFrequencyY)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETurbulenceElement, SVGNames::feTurbulenceTagString, SVGNames::numOctavesAttrString, long, NumOctaves, numOctaves)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETurbulenceElement, SVGNames::feTurbulenceTagString, SVGNames::seedAttrString, float, Seed, seed)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETurbulenceElement, SVGNames::feTurbulenceTagString, SVGNames::stitchTilesAttrString, int, StitchTiles, stitchTiles)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFETurbulenceElement, SVGNames::feTurbulenceTagString, SVGNames::typeAttrString, int, Type, type)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
