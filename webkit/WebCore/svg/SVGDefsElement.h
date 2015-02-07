

#ifndef SVGDefsElement_h
#define SVGDefsElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

    class SVGDefsElement : public SVGStyledTransformableElement,
                           public SVGTests,
                           public SVGLangSpace,
                           public SVGExternalResourcesRequired {
    public:
        SVGDefsElement(const QualifiedName&, Document*);
        virtual ~SVGDefsElement();

        virtual bool isValid() const;

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    protected:
        virtual const SVGElement* contextElement() const { return this; }
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
