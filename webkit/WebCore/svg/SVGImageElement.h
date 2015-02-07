

#ifndef SVGImageElement_h
#define SVGImageElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGImageLoader.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"
#include "SVGURIReference.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore {

    class SVGLength;

    class SVGImageElement : public SVGStyledTransformableElement,
                            public SVGTests,
                            public SVGLangSpace,
                            public SVGExternalResourcesRequired,
                            public SVGURIReference {
    public:
        SVGImageElement(const QualifiedName&, Document*);
        virtual ~SVGImageElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

        virtual void attach();
        virtual void insertedIntoDocument();

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
 
        virtual const QualifiedName& imageSourceAttributeName() const;       
        virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    protected:
        virtual bool haveLoadedRequiredResources();
        virtual bool hasRelativeValues() const;
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGImageElement, SVGNames::imageTagString, SVGNames::xAttrString, SVGLength, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGImageElement, SVGNames::imageTagString, SVGNames::yAttrString, SVGLength, Y, y)
        ANIMATED_PROPERTY_DECLARATIONS(SVGImageElement, SVGNames::imageTagString, SVGNames::widthAttrString, SVGLength, Width, width)
        ANIMATED_PROPERTY_DECLARATIONS(SVGImageElement, SVGNames::imageTagString, SVGNames::heightAttrString, SVGLength, Height, height)
        ANIMATED_PROPERTY_DECLARATIONS(SVGImageElement, SVGNames::imageTagString, SVGNames::preserveAspectRatioAttrString, SVGPreserveAspectRatio, PreserveAspectRatio, preserveAspectRatio)

        SVGImageLoader m_imageLoader;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
