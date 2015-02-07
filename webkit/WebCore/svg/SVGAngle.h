

#ifndef SVGAngle_h
#define SVGAngle_h

#if ENABLE(SVG)
#include "PlatformString.h"
#include "SVGNames.h"

namespace WebCore {

    class SVGStyledElement;

    class SVGAngle : public RefCounted<SVGAngle> {
    public:
        static PassRefPtr<SVGAngle> create()
        {
            return adoptRef(new SVGAngle);
        }
        virtual ~SVGAngle();
        
        enum SVGAngleType {
            SVG_ANGLETYPE_UNKNOWN           = 0,
            SVG_ANGLETYPE_UNSPECIFIED       = 1,
            SVG_ANGLETYPE_DEG               = 2,
            SVG_ANGLETYPE_RAD               = 3,
            SVG_ANGLETYPE_GRAD              = 4
        };

        SVGAngleType unitType() const;

        void setValue(float);
        float value() const; 

        void setValueInSpecifiedUnits(float valueInSpecifiedUnits);
        float valueInSpecifiedUnits() const;

        void setValueAsString(const String&);
        String valueAsString() const;

        void newValueSpecifiedUnits(unsigned short unitType, float valueInSpecifiedUnits);
        void convertToSpecifiedUnits(unsigned short unitType);

        // Throughout SVG 1.1 'SVGAngle' is only used for 'SVGMarkerElement' (orient-angle)
        const QualifiedName& associatedAttributeName() const { return SVGNames::orientAttr; }

    private:
        SVGAngle();

        SVGAngleType m_unitType;
        float m_value;
        float m_valueInSpecifiedUnits;
        mutable String m_valueAsString;

        void calculate();
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAngle_h
