

#ifndef RenderSVGImage_h
#define RenderSVGImage_h

#if ENABLE(SVG)

#include "TransformationMatrix.h"
#include "FloatRect.h"
#include "RenderImage.h"
#include "SVGRenderSupport.h"

namespace WebCore {

    class SVGImageElement;
    class SVGPreserveAspectRatio;

    class RenderSVGImage : public RenderImage, SVGRenderBase {
    public:
        RenderSVGImage(SVGImageElement*);

    private:
        virtual const char* renderName() const { return "RenderSVGImage"; }
        virtual bool isSVGImage() const { return true; }

        virtual TransformationMatrix localToParentTransform() const { return m_localTransform; }

        virtual FloatRect objectBoundingBox() const;
        virtual FloatRect repaintRectInLocalCoordinates() const;

        virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
        virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);

        virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;

        virtual void absoluteRects(Vector<IntRect>&, int tx, int ty);
        virtual void absoluteQuads(Vector<FloatQuad>&);
        virtual void addFocusRingRects(GraphicsContext*, int tx, int ty);

        virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);
        void adjustRectsForAspectRatio(FloatRect& destRect, FloatRect& srcRect, SVGPreserveAspectRatio*);
        
        virtual void layout();
        virtual void paint(PaintInfo&, int parentX, int parentY);

        virtual bool requiresLayer() const { return false; }

        virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
        virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

        virtual TransformationMatrix localTransform() const { return m_localTransform; }

        TransformationMatrix m_localTransform;
        FloatRect m_localBounds;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGImage_h

// vim:ts=4:noet
