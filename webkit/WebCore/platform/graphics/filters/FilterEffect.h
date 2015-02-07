

#ifndef FilterEffect_h
#define FilterEffect_h

#if ENABLE(FILTERS)
#include "Filter.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "TextStream.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class FilterEffect : public RefCounted<FilterEffect> {
    public:
        virtual ~FilterEffect();

        bool xBoundingBoxMode() const { return m_xBBoxMode; }
        void setXBoundingBoxMode(bool bboxMode) { m_xBBoxMode = bboxMode; }

        bool yBoundingBoxMode() const { return m_yBBoxMode; }
        void setYBoundingBoxMode(bool bboxMode) { m_yBBoxMode = bboxMode; }

        bool widthBoundingBoxMode() const { return m_widthBBoxMode; }
        void setWidthBoundingBoxMode(bool bboxMode) { m_widthBBoxMode = bboxMode; }

        bool heightBoundingBoxMode() const { return m_heightBBoxMode; }
        void setHeightBoundingBoxMode(bool bboxMode) { m_heightBBoxMode = bboxMode; }

        void setUnionOfChildEffectSubregions(const FloatRect& uniteRect) { m_unionOfChildEffectSubregions = uniteRect; }
        FloatRect unionOfChildEffectSubregions() const { return m_unionOfChildEffectSubregions; }

        FloatRect subRegion() const { return m_subRegion; }
        void setSubRegion(const FloatRect& subRegion) { m_subRegion = subRegion; }

        bool hasX() { return m_hasX; }
        void setHasX(bool value) { m_hasX = value; }

        bool hasY() { return m_hasY; }
        void setHasY(bool value) { m_hasY = value; }

        bool hasWidth() { return m_hasWidth; }
        void setHasWidth(bool value) { m_hasWidth = value; }

        bool hasHeight() { return m_hasHeight; }
        void setHasHeight(bool value) { m_hasHeight = value; }

        // The result is bounded by the size of the filter primitive to save resources
        ImageBuffer* resultImage() { return m_effectBuffer.get(); }
        void setEffectBuffer(PassOwnPtr<ImageBuffer> effectBuffer) { m_effectBuffer = effectBuffer; }

        FloatRect calculateUnionOfChildEffectSubregions(Filter*, FilterEffect*, FilterEffect*);
        FloatRect calculateUnionOfChildEffectSubregions(Filter*, FilterEffect*);

        GraphicsContext* getEffectContext();
        FloatRect calculateDrawingRect(const FloatRect&);

        virtual FloatRect uniteChildEffectSubregions(Filter* filter) { return filter->filterRegion(); }
        virtual FloatRect calculateEffectRect(Filter*);
        virtual void apply(Filter*) = 0;
        virtual void dump() = 0;

        virtual bool isSourceInput() { return false; }

        virtual TextStream& externalRepresentation(TextStream&) const;
    protected:
        FilterEffect();

    private:

        bool m_xBBoxMode : 1;
        bool m_yBBoxMode : 1;
        bool m_widthBBoxMode : 1;
        bool m_heightBBoxMode : 1;

        bool m_hasX : 1;
        bool m_hasY : 1;
        bool m_hasWidth : 1;
        bool m_hasHeight : 1;

        FloatRect m_subRegion;
        FloatRect m_unionOfChildEffectSubregions;

        mutable OwnPtr<ImageBuffer> m_effectBuffer;
    };

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FilterEffect_h
