

#ifndef SVGFEImage_h
#define SVGFEImage_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "CachedImage.h"
#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "FilterEffect.h"
#include "Filter.h"

namespace WebCore {

    class FEImage : public FilterEffect
                     , public CachedResourceClient {
    public:
        static PassRefPtr<FEImage> create(CachedImage*);
        virtual ~FEImage();

        // FIXME: We need to support <svg> (RenderObject*) as well as image data.

        CachedImage* cachedImage() const;
        void setCachedImage(CachedImage*);

        void apply(Filter*);
        void dump();
        TextStream& externalRepresentation(TextStream& ts) const;
        
    private:
        FEImage(CachedImage*);

        CachedResourceHandle<CachedImage> m_cachedImage;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFEImage_h
