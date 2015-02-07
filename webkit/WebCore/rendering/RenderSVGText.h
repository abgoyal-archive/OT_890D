

#ifndef RenderSVGText_h
#define RenderSVGText_h

#if ENABLE(SVG)

#include "TransformationMatrix.h"
#include "RenderSVGBlock.h"

namespace WebCore {

class SVGTextElement;

class RenderSVGText : public RenderSVGBlock {
public:
    RenderSVGText(SVGTextElement* node);

private:
    virtual const char* renderName() const { return "RenderSVGText"; }

    virtual bool isSVGText() const { return true; }

    virtual TransformationMatrix localToParentTransform() const { return m_localTransform; }

    virtual void paint(PaintInfo&, int tx, int ty);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);
    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);

    virtual bool requiresLayer() const { return false; }
    virtual void layout();

    virtual void absoluteRects(Vector<IntRect>&, int tx, int ty);
    virtual void absoluteQuads(Vector<FloatQuad>&);

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;

    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect repaintRectInLocalCoordinates() const;

    // FIXME: This can be removed when localTransform() is removed from RenderObject
    virtual TransformationMatrix localTransform() const { return m_localTransform; }

    virtual RootInlineBox* createRootInlineBox();

    TransformationMatrix m_localTransform;
};

}

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
