

#ifndef SVGFilter_h
#define SVGFilter_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "Filter.h"
#include "FilterEffect.h"
#include "FloatRect.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class SVGFilter : public Filter {
    public:
        static PassRefPtr<SVGFilter> create(const FloatRect&, const FloatRect&, bool, bool);

        bool effectBoundingBoxMode() { return m_effectBBoxMode; }

        FloatRect filterRegion() { return m_filterRect; }
        FloatRect sourceImageRect() { return m_itemBox; }
        void calculateEffectSubRegion(FilterEffect*);

    private:
        SVGFilter(const FloatRect& itemBox, const FloatRect& filterRect, bool itemBBoxMode, bool filterBBoxMode);

        FloatRect m_itemBox;
        FloatRect m_filterRect;
        bool m_effectBBoxMode;
        bool m_filterBBoxMode;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFilter_h
