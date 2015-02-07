

#ifndef Filter_h
#define Filter_h

#if ENABLE(FILTERS)
#include "FloatRect.h"
#include "ImageBuffer.h"
#include "StringHash.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class FilterEffect;

    class Filter : public RefCounted<Filter> {
    public:
        virtual ~Filter() { }

        void setSourceImage(PassOwnPtr<ImageBuffer> sourceImage) { m_sourceImage = sourceImage; }
        ImageBuffer* sourceImage() { return m_sourceImage.get(); }

        virtual FloatRect sourceImageRect() = 0;
        virtual FloatRect filterRegion() = 0;

        // SVG specific
        virtual void calculateEffectSubRegion(FilterEffect*) = 0;
        virtual bool effectBoundingBoxMode() = 0;

    private:
        OwnPtr<ImageBuffer> m_sourceImage;
    };

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // Filter_h
