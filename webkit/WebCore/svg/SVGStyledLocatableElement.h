

#ifndef SVGStyledLocatableElement_h
#define SVGStyledLocatableElement_h

#if ENABLE(SVG)
#include "SVGLocatable.h"
#include "SVGStyledElement.h"

namespace WebCore {

    class SVGElement;

    class SVGStyledLocatableElement : public SVGStyledElement,
                                      virtual public SVGLocatable {
    public:
        SVGStyledLocatableElement(const QualifiedName&, Document*);
        virtual ~SVGStyledLocatableElement();
        
        virtual bool isStyledLocatable() const { return true; }

        virtual SVGElement* nearestViewportElement() const;
        virtual SVGElement* farthestViewportElement() const;

        virtual FloatRect getBBox() const;
        virtual TransformationMatrix getCTM() const;
        virtual TransformationMatrix getScreenCTM() const;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGStyledLocatableElement_h
