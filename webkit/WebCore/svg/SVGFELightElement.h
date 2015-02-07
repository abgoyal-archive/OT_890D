

#ifndef SVGFELightElement_h
#define SVGFELightElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGElement.h"
#include "SVGLightSource.h"

namespace WebCore {

    extern char SVGFELightElementIdentifier[];

    class SVGFELightElement : public SVGElement {
    public:
        SVGFELightElement(const QualifiedName&, Document*);
        virtual ~SVGFELightElement();
        
        virtual LightSource* lightSource() const = 0;
        virtual void parseMappedAttribute(MappedAttribute*);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::azimuthAttrString, float, Azimuth, azimuth)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::elevationAttrString, float, Elevation, elevation)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::xAttrString, float, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::yAttrString, float, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::zAttrString, float, Z, z)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::pointsAtXAttrString, float, PointsAtX, pointsAtX)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::pointsAtYAttrString, float, PointsAtY, pointsAtY)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::pointsAtZAttrString, float, PointsAtZ, pointsAtZ)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::specularExponentAttrString, float, SpecularExponent, specularExponent)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFELightElement, SVGFELightElementIdentifier, SVGNames::limitingConeAngleAttrString, float, LimitingConeAngle, limitingConeAngle)
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
#endif
