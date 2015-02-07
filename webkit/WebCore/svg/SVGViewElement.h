

#ifndef SVGViewElement_h
#define SVGViewElement_h

#if ENABLE(SVG)
#include "SVGStyledElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGZoomAndPan.h"

namespace WebCore {

    class SVGStringList;
    class SVGViewElement : public SVGStyledElement,
                           public SVGExternalResourcesRequired,
                           public SVGFitToViewBox,
                           public SVGZoomAndPan {
    public:
        SVGViewElement(const QualifiedName&, Document*);
        virtual ~SVGViewElement();

        virtual void parseMappedAttribute(MappedAttribute*);

        SVGStringList* viewTarget() const;

        virtual bool rendererIsNeeded(RenderStyle*) { return false; }

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        mutable RefPtr<SVGStringList> m_viewTarget;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
