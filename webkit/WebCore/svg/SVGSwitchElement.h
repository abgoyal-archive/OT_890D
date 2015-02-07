

#ifndef SVGSwitchElement_h
#define SVGSwitchElement_h
#if ENABLE(SVG)

#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {
    class SVGSwitchElement : public SVGStyledTransformableElement,
                             public SVGTests,
                             public SVGLangSpace,
                             public SVGExternalResourcesRequired {
    public:
        SVGSwitchElement(const QualifiedName&, Document*);
        virtual ~SVGSwitchElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual bool childShouldCreateRenderer(Node*) const;

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        mutable bool m_insideRenderSection;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
