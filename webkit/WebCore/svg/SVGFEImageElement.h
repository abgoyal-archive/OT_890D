

#ifndef SVGFEImageElement_h
#define SVGFEImageElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "CachedResourceHandle.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGURIReference.h"
#include "SVGLangSpace.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFEImage.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore {

    class SVGFEImageElement : public SVGFilterPrimitiveStandardAttributes,
                              public SVGURIReference,
                              public SVGLangSpace,
                              public SVGExternalResourcesRequired,
                              public CachedResourceClient {
    public:
        SVGFEImageElement(const QualifiedName&, Document*);
        virtual ~SVGFEImageElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void notifyFinished(CachedResource*);

        virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;
        virtual bool build(SVGResourceFilter*);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEImageElement, SVGNames::feImageTagString, SVGNames::preserveAspectRatioAttrString, SVGPreserveAspectRatio, PreserveAspectRatio, preserveAspectRatio)

        CachedResourceHandle<CachedImage> m_cachedImage;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
