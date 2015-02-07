

#ifndef SVGViewSpec_h
#define SVGViewSpec_h

#if ENABLE(SVG)
#include "SVGFitToViewBox.h"
#include "SVGZoomAndPan.h"

#include <wtf/RefPtr.h>

namespace WebCore {

    class SVGElement;
    class SVGSVGElement;
    class SVGTransformList;

    class SVGViewSpec : public SVGFitToViewBox,
                        public SVGZoomAndPan {
    public:
        SVGViewSpec(const SVGSVGElement*);
        virtual ~SVGViewSpec();

        bool parseViewSpec(const String&);

        void setTransform(const String&);
        SVGTransformList* transform() const { return m_transform.get(); }

        void setViewBoxString(const String&);

        void setPreserveAspectRatioString(const String&);

        void setViewTargetString(const String&);
        String viewTargetString() const { return m_viewTargetString; }
        SVGElement* viewTarget() const;

        virtual const SVGElement* contextElement() const;

    private:
        mutable RefPtr<SVGTransformList> m_transform;
        const SVGSVGElement* m_contextElement;
        String m_viewTargetString;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
