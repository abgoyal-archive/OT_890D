

#ifndef RenderForeignObject_h
#define RenderForeignObject_h
#if ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)

#include "TransformationMatrix.h"
#include "RenderSVGBlock.h"

namespace WebCore {

class SVGForeignObjectElement;

class RenderForeignObject : public RenderSVGBlock {
public:
    RenderForeignObject(SVGForeignObjectElement*);

    virtual const char* renderName() const { return "RenderForeignObject"; }

    virtual void paint(PaintInfo&, int parentX, int parentY);

    virtual TransformationMatrix localToParentTransform() const;

    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);
    virtual bool requiresLayer() const { return false; }
    virtual void layout();

    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect repaintRectInLocalCoordinates() const;

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

 private:
    TransformationMatrix translationForAttributes() const;

    virtual TransformationMatrix localTransform() const { return m_localTransform; }

    TransformationMatrix m_localTransform;
};

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#endif // RenderForeignObject_h
