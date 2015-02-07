

#ifndef SVGRenderBase_h
#define SVGRenderBase_h

#if ENABLE(SVG)
#include "RenderObject.h"

namespace WebCore {

    class SVGResourceFilter;
    class ImageBuffer;

    // SVGRendererBase is an abstract base class which all SVG renderers inherit
    // from in order to share SVG renderer code.
    // FIXME: This code can all move into RenderSVGModelObject once
    // all SVG renderers inherit from RenderSVGModelObject.
    class SVGRenderBase {
    public:
        // FIXME: These are only public for SVGRootInlineBox.
        // It's unclear if these should be exposed or not.  SVGRootInlineBox may
        // pass the wrong RenderObject* and boundingBox to these functions.
        static void prepareToRenderSVGContent(RenderObject*, RenderObject::PaintInfo&, const FloatRect& boundingBox, SVGResourceFilter*&, SVGResourceFilter* rootFilter = 0);
        static void finishRenderSVGContent(RenderObject*, RenderObject::PaintInfo&, SVGResourceFilter*&, GraphicsContext* savedContext);

    protected:
        static IntRect clippedOverflowRectForRepaint(RenderObject*, RenderBoxModelObject* repaintContainer);
        static void computeRectForRepaint(RenderObject*, RenderBoxModelObject* repaintContainer, IntRect&, bool fixed);

        static void mapLocalToContainer(const RenderObject*, RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&);

        // Used to share the "walk all the children" logic between objectBoundingBox
        // and repaintRectInLocalCoordinates in RenderSVGRoot and RenderSVGContainer
        static FloatRect computeContainerBoundingBox(const RenderObject* container, bool includeAllPaintedContent);

        // returns the filter bounding box (or the empty rect if no filter) in local coordinates
        static FloatRect filterBoundingBoxForRenderer(const RenderObject*);
    };

    // FIXME: This should move to RenderObject or PaintInfo
    // Used for transforming the GraphicsContext and damage rect before passing PaintInfo to child renderers.
    void applyTransformToPaintInfo(RenderObject::PaintInfo&, const TransformationMatrix& localToChildTransform);

    // This offers a way to render parts of a WebKit rendering tree into a ImageBuffer.
    void renderSubtreeToImage(ImageBuffer*, RenderObject*);

    void clampImageBufferSizeToViewport(FrameView*, IntSize& imageBufferSize);
} // namespace WebCore

#endif // ENABLE(SVG)

#endif // SVGRenderBase_h
