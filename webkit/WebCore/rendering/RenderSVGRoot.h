

#ifndef RenderSVGRoot_h
#define RenderSVGRoot_h

#if ENABLE(SVG)
#include "RenderBox.h"
#include "FloatRect.h"
#include "SVGRenderSupport.h"

namespace WebCore {

class SVGStyledElement;
class TransformationMatrix;

class RenderSVGRoot : public RenderBox, SVGRenderBase {
public:
    RenderSVGRoot(SVGStyledElement*);

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual bool isSVGRoot() const { return true; }
    virtual const char* renderName() const { return "RenderSVGRoot"; }

    virtual int lineHeight(bool b, bool isRootLineBox = false) const;
    virtual int baselinePosition(bool b, bool isRootLineBox = false) const;
    virtual void calcPrefWidths();

    virtual void layout();
    virtual void paint(PaintInfo&, int parentX, int parentY);

    virtual TransformationMatrix localToParentTransform() const;

    bool fillContains(const FloatPoint&) const;
    bool strokeContains(const FloatPoint&) const;

    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect repaintRectInLocalCoordinates() const;

    // FIXME: Both of these overrides should be removed.
    virtual TransformationMatrix localTransform() const;
    virtual TransformationMatrix absoluteTransform() const;

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& repaintRect, bool fixed);

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;

    void calcViewport();
    const FloatSize& viewportSize() const;

    bool selfWillPaint() const;

    IntSize parentOriginToBorderBox() const;
    IntSize borderOriginToContentBox() const;
    TransformationMatrix localToRepaintContainerTransform(const IntPoint& parentOriginInContainer) const;
    TransformationMatrix localToBorderBoxTransform() const;

    RenderObjectChildList m_children;
    FloatSize m_viewportSize;
};

inline RenderSVGRoot* toRenderSVGRoot(RenderObject* object)
{ 
    ASSERT(!object || object->isSVGRoot());
    return static_cast<RenderSVGRoot*>(object);
}

inline const RenderSVGRoot* toRenderSVGRoot(const RenderObject* object)
{ 
    ASSERT(!object || object->isSVGRoot());
    return static_cast<const RenderSVGRoot*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGRoot(const RenderSVGRoot*);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGRoot_h

// vim:ts=4:noet
