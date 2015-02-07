

#ifndef SVGPatternElement_h
#define SVGPatternElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGLangSpace.h"
#include "SVGPaintServerPattern.h"
#include "SVGStyledElement.h"
#include "SVGTests.h"
#include "SVGTransformList.h"
#include "SVGURIReference.h"

namespace WebCore {

    struct PatternAttributes;
 
    class SVGLength;

    class SVGPatternElement : public SVGStyledElement,
                              public SVGURIReference,
                              public SVGTests,
                              public SVGLangSpace,
                              public SVGExternalResourcesRequired,
                              public SVGFitToViewBox {
    public:
        SVGPatternElement(const QualifiedName&, Document*);
        virtual ~SVGPatternElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
        virtual SVGResource* canvasResource();

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::xAttrString, SVGLength, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::yAttrString, SVGLength, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::widthAttrString, SVGLength, Width, width)
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::heightAttrString, SVGLength, Height, height)
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::patternUnitsAttrString, int, PatternUnits, patternUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::patternContentUnitsAttrString, int, PatternContentUnits, patternContentUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGPatternElement, SVGNames::patternTagString, SVGNames::patternTransformAttrString, SVGTransformList, PatternTransform, patternTransform)

        mutable RefPtr<SVGPaintServerPattern> m_resource;

    private:
        friend class SVGPaintServerPattern;
        void buildPattern(const FloatRect& targetRect) const;

        PatternAttributes collectPatternProperties() const;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
