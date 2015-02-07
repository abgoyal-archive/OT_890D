

#ifndef SVGGradientElement_h
#define SVGGradientElement_h

#if ENABLE(SVG)
#include "SVGPaintServerGradient.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGStyledElement.h"
#include "SVGTransformList.h"
#include "SVGURIReference.h"

namespace WebCore {

    extern char SVGGradientElementIdentifier[];

    class SVGGradientElement : public SVGStyledElement,
                               public SVGURIReference,
                               public SVGExternalResourcesRequired {
    public:
        SVGGradientElement(const QualifiedName&, Document*);
        virtual ~SVGGradientElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);
        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

        virtual SVGResource* canvasResource();

    protected:
        friend class SVGPaintServerGradient;
        friend class SVGLinearGradientElement;
        friend class SVGRadialGradientElement;

        virtual void buildGradient() const = 0;
        virtual SVGPaintServerType gradientType() const = 0;

        Vector<SVGGradientStop> buildStops() const;
        mutable RefPtr<SVGPaintServerGradient> m_resource;
 
        virtual const SVGElement* contextElement() const { return this; }

    protected:
        ANIMATED_PROPERTY_DECLARATIONS(SVGGradientElement, SVGGradientElementIdentifier, SVGNames::spreadMethodAttrString, int, SpreadMethod, spreadMethod)
        ANIMATED_PROPERTY_DECLARATIONS(SVGGradientElement, SVGGradientElementIdentifier, SVGNames::gradientUnitsAttrString, int, GradientUnits, gradientUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGGradientElement, SVGGradientElementIdentifier, SVGNames::gradientTransformAttrString, SVGTransformList, GradientTransform, gradientTransform)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
