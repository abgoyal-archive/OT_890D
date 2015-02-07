

#ifndef SVGResourceMasker_h
#define SVGResourceMasker_h

#if ENABLE(SVG)

#include "GraphicsContext.h"
#include "SVGResource.h"

#include <memory>

#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class FloatRect;
    class ImageBuffer;
    class SVGMaskElement;

    class SVGResourceMasker : public SVGResource {
    public:
        static PassRefPtr<SVGResourceMasker> create(const SVGMaskElement* ownerElement) { return adoptRef(new SVGResourceMasker(ownerElement)); }
        virtual ~SVGResourceMasker();
        
        virtual void invalidate();
        
        virtual SVGResourceType resourceType() const { return MaskerResourceType; }
        virtual TextStream& externalRepresentation(TextStream&) const;

        // To be implemented by the specific rendering devices
        void applyMask(GraphicsContext*, const FloatRect& boundingBox);

    private:
        SVGResourceMasker(const SVGMaskElement*);

        const SVGMaskElement* m_ownerElement;
        
        OwnPtr<ImageBuffer> m_mask;
        FloatRect m_maskRect;
    };

    SVGResourceMasker* getMaskerById(Document*, const AtomicString&);

} // namespace WebCore

#endif

#endif // SVGResourceMasker_h
