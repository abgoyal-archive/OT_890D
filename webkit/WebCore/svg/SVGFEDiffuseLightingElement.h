

#ifndef SVGFEDiffuseLightingElement_h
#define SVGFEDiffuseLightingElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFELightElement.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

    extern char SVGKernelUnitLengthXIdentifier[];
    extern char SVGKernelUnitLengthYIdentifier[];

    class FEDiffuseLighting;
    class SVGColor;

    class SVGFEDiffuseLightingElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEDiffuseLightingElement(const QualifiedName&, Document*);
        virtual ~SVGFEDiffuseLightingElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDiffuseLightingElement, SVGNames::feDiffuseLightingTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDiffuseLightingElement, SVGNames::feDiffuseLightingTagString, SVGNames::diffuseConstantAttrString, float, DiffuseConstant, diffuseConstant)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDiffuseLightingElement, SVGNames::feDiffuseLightingTagString, SVGNames::surfaceScaleAttrString, float, SurfaceScale, surfaceScale)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDiffuseLightingElement, SVGNames::feDiffuseLightingTagString, SVGKernelUnitLengthXIdentifier, float, KernelUnitLengthX, kernelUnitLengthX)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDiffuseLightingElement, SVGNames::feDiffuseLightingTagString, SVGKernelUnitLengthYIdentifier, float, KernelUnitLengthY, kernelUnitLengthY)

        LightSource* findLights() const;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
