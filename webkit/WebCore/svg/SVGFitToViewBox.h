

#ifndef SVGFitToViewBox_h
#define SVGFitToViewBox_h

#if ENABLE(SVG)
#include "SVGElement.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore {

    extern char SVGFitToViewBoxIdentifier[];

    class TransformationMatrix;

    class SVGFitToViewBox {
    public:
        SVGFitToViewBox();
        virtual ~SVGFitToViewBox();

        bool parseViewBox(const UChar*& start, const UChar* end, float& x, float& y, float& w, float& h, bool validate = true);
        virtual TransformationMatrix viewBoxToViewTransform(float viewWidth, float viewHeight) const;

        bool parseMappedAttribute(MappedAttribute*);
        bool isKnownAttribute(const QualifiedName&);

        virtual const SVGElement* contextElement() const = 0;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFitToViewBox, SVGFitToViewBoxIdentifier, SVGNames::viewBoxAttrString, FloatRect, ViewBox, viewBox)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFitToViewBox, SVGFitToViewBoxIdentifier, SVGNames::preserveAspectRatioAttrString, SVGPreserveAspectRatio, PreserveAspectRatio, preserveAspectRatio)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGFitToViewBox_h
