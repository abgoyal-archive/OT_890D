

#ifndef SVGFEFlood_h
#define SVGFEFlood_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "Color.h"
#include "Filter.h"
#include "FilterEffect.h"

namespace WebCore {

    class FEFlood : public FilterEffect {
    public:
        static PassRefPtr<FEFlood> create(FilterEffect*, const Color&, const float&);

        Color floodColor() const;
        void setFloodColor(const Color &);

        float floodOpacity() const;
        void setFloodOpacity(float);

        void apply(Filter*);
        void dump();
        TextStream& externalRepresentation(TextStream& ts) const;

    private:
        FEFlood(FilterEffect*, const Color&, const float&);

        RefPtr<FilterEffect> m_in;
        Color m_floodColor;
        float m_floodOpacity;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFEFlood_h
