

#ifndef SVGStopElement_h
#define SVGStopElement_h

#if ENABLE(SVG)
#include "SVGStyledElement.h"

namespace WebCore {

    class SVGStopElement : public SVGStyledElement {
    public:
        SVGStopElement(const QualifiedName&, Document*);
        virtual ~SVGStopElement();

        virtual bool isGradientStop() const { return true; }

        virtual void parseMappedAttribute(MappedAttribute*);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGStopElement, SVGNames::stopTagString, SVGNames::offsetAttrString, float, Offset, offset)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
