

#ifndef SVGStyledElement_h
#define SVGStyledElement_h

#if ENABLE(SVG)
#include "HTMLNames.h"
#include "SVGElement.h"
#include "SVGStylable.h"

namespace WebCore {

    extern char SVGStyledElementIdentifier[];
    class SVGResource;

    class SVGStyledElement : public SVGElement,
                             public SVGStylable {
    public:
        SVGStyledElement(const QualifiedName&, Document*);
        virtual ~SVGStyledElement();
        
        virtual bool isStyled() const { return true; }
        virtual bool supportsMarkers() const { return false; }

        virtual PassRefPtr<CSSValue> getPresentationAttribute(const String& name);
        virtual CSSStyleDeclaration* style() { return StyledElement::style(); }

        bool isKnownAttribute(const QualifiedName&);

        virtual bool rendererIsNeeded(RenderStyle*);
        virtual SVGResource* canvasResource() { return 0; }
        
        virtual bool mapToEntry(const QualifiedName&, MappedAttributeEntry&) const;
        virtual void parseMappedAttribute(MappedAttribute*);

        virtual void svgAttributeChanged(const QualifiedName&);

        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

        // Centralized place to force a manual style resolution. Hacky but needed for now.
        PassRefPtr<RenderStyle> resolveStyle(RenderStyle* parentStyle);

        void invalidateResourcesInAncestorChain() const;        
        virtual void detach();
                                 
        void setInstanceUpdatesBlocked(bool);
        
    protected:
        virtual bool hasRelativeValues() const { return true; }
        
        static int cssPropertyIdForSVGAttributeName(const QualifiedName&);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGStyledElement, SVGStyledElementIdentifier, HTMLNames::classAttrString, String, ClassName, className)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGStyledElement
