

#ifndef RenderPath_h
#define RenderPath_h

#if ENABLE(SVG)

#include "FloatRect.h"
#include "RenderSVGModelObject.h"
#include "TransformationMatrix.h"

namespace WebCore {

class FloatPoint;
class RenderSVGContainer;
class SVGStyledTransformableElement;

class RenderPath : public RenderSVGModelObject {
public:
    RenderPath(SVGStyledTransformableElement*);

    const Path& path() const;

private:
    // Hit-detection seperated for the fill and the stroke
    bool fillContains(const FloatPoint&, bool requiresFill = true) const;
    bool strokeContains(const FloatPoint&, bool requiresStroke = true) const;

    virtual FloatRect objectBoundingBox() const;
    virtual FloatRect repaintRectInLocalCoordinates() const;

    virtual TransformationMatrix localToParentTransform() const;

    void setPath(const Path&);

    virtual bool isRenderPath() const { return true; }
    virtual const char* renderName() const { return "RenderPath"; }

    virtual void layout();
    virtual void paint(PaintInfo&, int parentX, int parentY);
    virtual void addFocusRingRects(GraphicsContext*, int tx, int ty);

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);

    FloatRect drawMarkersIfNeeded(GraphicsContext*, const FloatRect&, const Path&) const;

private:
    virtual TransformationMatrix localTransform() const;

    mutable Path m_path;
    mutable FloatRect m_cachedLocalFillBBox;
    mutable FloatRect m_cachedLocalRepaintRect;
    FloatRect m_markerBounds;
    TransformationMatrix m_localTransform;
};

inline RenderPath* toRenderPath(RenderObject* object)
{
    ASSERT(!object || object->isRenderPath());
    return static_cast<RenderPath*>(object);
}

inline const RenderPath* toRenderPath(const RenderObject* object)
{
    ASSERT(!object || object->isRenderPath());
    return static_cast<const RenderPath*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderPath(const RenderPath*);

}

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
