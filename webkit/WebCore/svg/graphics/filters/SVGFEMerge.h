

#ifndef SVGFEMerge_h
#define SVGFEMerge_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FilterEffect.h"
#include "Filter.h"
#include <wtf/Vector.h>

namespace WebCore {

    class FEMerge : public FilterEffect {
    public:
        static PassRefPtr<FEMerge> create(const Vector<FilterEffect*>&);

        const Vector<FilterEffect*>& mergeInputs() const;
        void setMergeInputs(const Vector<FilterEffect*>& mergeInputs);

        virtual FloatRect uniteChildEffectSubregions(Filter*);
        void apply(Filter*);
        void dump();
        TextStream& externalRepresentation(TextStream& ts) const;

    private:
        FEMerge(const Vector<FilterEffect*>&);

        Vector<FilterEffect*> m_mergeInputs;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFEMerge_h
