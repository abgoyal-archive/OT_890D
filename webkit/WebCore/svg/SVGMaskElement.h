

#ifndef SVGMaskElement_h
#define SVGMaskElement_h

#if ENABLE(SVG)
#include "SVGResourceMasker.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledLocatableElement.h"
#include "SVGTests.h"
#include "SVGURIReference.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

    class SVGLength;

    class SVGMaskElement : public SVGStyledLocatableElement,
                           public SVGURIReference,
                           public SVGTests,
                           public SVGLangSpace,
                           public SVGExternalResourcesRequired {
    public:
        SVGMaskElement(const QualifiedName&, Document*);
        virtual ~SVGMaskElement();
        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
        virtual SVGResource* canvasResource();

        PassOwnPtr<ImageBuffer> drawMaskerContent(const FloatRect& targetRect, FloatRect& maskRect) const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGMaskElement, SVGNames::maskTagString, SVGNames::maskUnitsAttrString, int, MaskUnits, maskUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMaskElement, SVGNames::maskTagString, SVGNames::maskContentUnitsAttrString, int, MaskContentUnits, maskContentUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMaskElement, SVGNames::maskTagString, SVGNames::xAttrString, SVGLength, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMaskElement, SVGNames::maskTagString, SVGNames::yAttrString, SVGLength, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMaskElement, SVGNames::maskTagString, SVGNames::widthAttrString, SVGLength, Width, width)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMaskElement, SVGNames::maskTagString, SVGNames::heightAttrString, SVGLength, Height, height)

        RefPtr<SVGResourceMasker> m_masker;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
