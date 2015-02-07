

#ifndef SVGLocatable_h
#define SVGLocatable_h

#if ENABLE(SVG)

#include "ExceptionCode.h"

namespace WebCore {

    class TransformationMatrix;
    class FloatRect;
    class SVGElement;

    class SVGLocatable {
    public:
        SVGLocatable();
        virtual ~SVGLocatable();

        // 'SVGLocatable' functions
        virtual SVGElement* nearestViewportElement() const = 0;
        virtual SVGElement* farthestViewportElement() const = 0;

        virtual FloatRect getBBox() const = 0;
        virtual TransformationMatrix getCTM() const = 0;
        virtual TransformationMatrix getScreenCTM() const = 0;
        TransformationMatrix getTransformToElement(SVGElement*, ExceptionCode&) const;

        static SVGElement* nearestViewportElement(const SVGElement*);
        static SVGElement* farthestViewportElement(const SVGElement*);

    protected:
        static FloatRect getBBox(const SVGElement*);
        static TransformationMatrix getCTM(const SVGElement*);
        static TransformationMatrix getScreenCTM(const SVGElement*);
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGLocatable_h

// vim:ts=4:noet
