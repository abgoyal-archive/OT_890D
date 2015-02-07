

#ifndef SVGSpotLightSource_h
#define SVGSpotLightSource_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FloatPoint3D.h"
#include "SVGLightSource.h"

namespace WebCore {

    class SpotLightSource : public LightSource {
    public:
        SpotLightSource(const FloatPoint3D& position, const FloatPoint3D& direction, float specularExponent, float limitingConeAngle)
            : LightSource(LS_SPOT)
            , m_position(position)
            , m_direction(direction)
            , m_specularExponent(specularExponent)
            , m_limitingConeAngle(limitingConeAngle)
        { }

        const FloatPoint3D& position() const { return m_position; }
        const FloatPoint3D& direction() const { return m_direction; }

        float specularExponent() const { return m_specularExponent; }
        float limitingConeAngle() const { return m_limitingConeAngle; }

        virtual TextStream& externalRepresentation(TextStream&) const;

    private:
        FloatPoint3D m_position;
        FloatPoint3D m_direction;

        float m_specularExponent;
        float m_limitingConeAngle;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGSpotLightSource_h
