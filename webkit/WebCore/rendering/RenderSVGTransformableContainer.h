

#ifndef RenderSVGTransformableContainer_h
#define RenderSVGTransformableContainer_h

#if ENABLE(SVG)
#include "RenderSVGContainer.h"

namespace WebCore {
    
    class SVGStyledTransformableElement;
    class RenderSVGTransformableContainer : public RenderSVGContainer {
    public:
        RenderSVGTransformableContainer(SVGStyledTransformableElement*);

        virtual TransformationMatrix localToParentTransform() const;

    private:
        virtual void calculateLocalTransform();
        // FIXME: This can be made non-virtual once SVGRenderTreeAsText stops using localTransform()
        virtual TransformationMatrix localTransform() const;

        TransformationMatrix m_localTransform;
    };
}

#endif // ENABLE(SVG)
#endif // RenderSVGTransformableContainer_h
