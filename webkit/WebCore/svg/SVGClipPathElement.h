

#ifndef SVGClipPathElement_h
#define SVGClipPathElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGResourceClipper.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

    class SVGClipPathElement : public SVGStyledTransformableElement,
                               public SVGTests,
                               public SVGLangSpace,
                               public SVGExternalResourcesRequired {
    public:
        SVGClipPathElement(const QualifiedName&, Document*);
        virtual ~SVGClipPathElement();

        virtual bool isValid() const { return SVGTests::isValid(); }
        virtual bool rendererIsNeeded(RenderStyle*) { return false; }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

        virtual SVGResource* canvasResource();

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGClipPathElement, SVGNames::clipPathTagString, SVGNames::clipPathUnitsAttrString, int, ClipPathUnits, clipPathUnits)

        RefPtr<SVGResourceClipper> m_clipper;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
