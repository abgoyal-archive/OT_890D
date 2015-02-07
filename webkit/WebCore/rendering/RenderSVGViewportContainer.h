

#ifndef RenderSVGViewportContainer_h
#define RenderSVGViewportContainer_h

#if ENABLE(SVG)

#include "RenderSVGContainer.h"

namespace WebCore {

// This is used for non-root <svg> elements and <marker> elements, neither of which are SVGTransformable
// thus we inherit from RenderSVGContainer instead of RenderSVGTransformableContainer
class RenderSVGViewportContainer : public RenderSVGContainer {
public:
    RenderSVGViewportContainer(SVGStyledElement*);

    // FIXME: This is only public for SVGResourceMarker::draw, likely the callsite should be changed.
    TransformationMatrix viewportTransform() const;

    virtual void paint(PaintInfo&, int parentX, int parentY);

private:
    virtual bool isSVGContainer() const { return true; }
    virtual const char* renderName() const { return "RenderSVGViewportContainer"; }

    virtual TransformationMatrix localToParentTransform() const;

    // FIXME: This override should be removed once callers of RenderBox::absoluteTransform() can be removed.
    virtual TransformationMatrix absoluteTransform() const;

    virtual void calcViewport();

    virtual void applyViewportClip(PaintInfo&);
    virtual bool pointIsInsideViewportClip(const FloatPoint& pointInParent);

    FloatRect m_viewport;
};
  
inline RenderSVGViewportContainer* toRenderSVGViewportContainer(RenderObject* object)
{
    ASSERT(!object || !strcmp(object->renderName(), "RenderSVGViewportContainer"));
    return static_cast<RenderSVGViewportContainer*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGViewportContainer(const RenderSVGViewportContainer*);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGViewportContainer_h

// vim:ts=4:noet
