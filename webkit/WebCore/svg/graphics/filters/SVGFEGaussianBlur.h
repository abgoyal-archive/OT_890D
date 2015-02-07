

#ifndef SVGFEGaussianBlur_h
#define SVGFEGaussianBlur_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FilterEffect.h"
#include "Filter.h"

namespace WebCore {

    class FEGaussianBlur : public FilterEffect {
    public:
        static PassRefPtr<FEGaussianBlur> create(FilterEffect*, const float&, const float&);

        float stdDeviationX() const;
        void setStdDeviationX(float);

        float stdDeviationY() const;
        void setStdDeviationY(float);

        virtual FloatRect uniteChildEffectSubregions(Filter* filter) { return calculateUnionOfChildEffectSubregions(filter, m_in.get()); }
        void apply(Filter*);
        void dump();
        TextStream& externalRepresentation(TextStream& ts) const;

    private:
        FEGaussianBlur(FilterEffect*, const float&, const float&);

        RefPtr<FilterEffect> m_in;
        float m_x;
        float m_y;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFEGaussianBlur_h
