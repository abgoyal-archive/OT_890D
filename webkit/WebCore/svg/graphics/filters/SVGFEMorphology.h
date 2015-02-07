

#ifndef SVGFEMorphology_h
#define SVGFEMorphology_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FilterEffect.h"
#include "Filter.h"

namespace WebCore {

    enum MorphologyOperatorType {
        FEMORPHOLOGY_OPERATOR_UNKNOWN = 0,
        FEMORPHOLOGY_OPERATOR_ERODE   = 1,
        FEMORPHOLOGY_OPERATOR_DIALATE = 2
    };

    class FEMorphology : public FilterEffect {
    public:
        PassRefPtr<FEMorphology> create(FilterEffect*, MorphologyOperatorType, const float&, const float&);  
        MorphologyOperatorType morphologyOperator() const;
        void setMorphologyOperator(MorphologyOperatorType);

        float radiusX() const;
        void setRadiusX(float);

        float radiusY() const;
        void setRadiusY(float);

        virtual FloatRect uniteChildEffectSubregions(Filter* filter) { return calculateUnionOfChildEffectSubregions(filter, m_in.get()); }
        void apply(Filter*);
        void dump();
        TextStream& externalRepresentation(TextStream& ts) const;

    private:
        FEMorphology(FilterEffect*, MorphologyOperatorType, const float&, const float&);
        
        RefPtr<FilterEffect> m_in;
        MorphologyOperatorType m_type;
        float m_radiusX;
        float m_radiusY;
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFEMorphology_h
