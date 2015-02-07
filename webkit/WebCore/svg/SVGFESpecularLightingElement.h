

#ifndef SVGFESpecularLightingElement_h
#define SVGFESpecularLightingElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFESpecularLighting.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

    extern char SVGKernelUnitLengthXIdentifier[];
    extern char SVGKernelUnitLengthYIdentifier[];

    class SVGColor;
    
    class SVGFESpecularLightingElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFESpecularLightingElement(const QualifiedName&, Document*);
        virtual ~SVGFESpecularLightingElement();
        
        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFESpecularLightingElement, SVGNames::feSpecularLightingTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFESpecularLightingElement, SVGNames::feSpecularLightingTagString, SVGNames::specularConstantAttrString, float, SpecularConstant, specularConstant)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFESpecularLightingElement, SVGNames::feSpecularLightingTagString, SVGNames::specularExponentAttrString, float, SpecularExponent, specularExponent)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFESpecularLightingElement, SVGNames::feSpecularLightingTagString, SVGNames::surfaceScaleAttrString, float, SurfaceScale, surfaceScale)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFESpecularLightingElement, SVGNames::feSpecularLightingTagString, SVGKernelUnitLengthXIdentifier, float, KernelUnitLengthX, kernelUnitLengthX)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFESpecularLightingElement, SVGNames::feSpecularLightingTagString, SVGKernelUnitLengthYIdentifier, float, KernelUnitLengthY, kernelUnitLengthY)
        
        LightSource* findLights() const;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
