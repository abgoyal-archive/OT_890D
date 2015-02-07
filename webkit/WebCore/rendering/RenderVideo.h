

#ifndef RenderVideo_h
#define RenderVideo_h

#if ENABLE(VIDEO)

#include "RenderMedia.h"

namespace WebCore {
    
class HTMLMediaElement;
#if USE(ACCELERATED_COMPOSITING)
class GraphicsLayer;
#endif

class RenderVideo : public RenderMedia {
public:
    RenderVideo(HTMLMediaElement*);
    virtual ~RenderVideo();

    void videoSizeChanged();
    IntRect videoBox() const;
    
#if USE(ACCELERATED_COMPOSITING)
    bool supportsAcceleratedRendering() const;
    void acceleratedRenderingStateChanged();
    GraphicsLayer* videoGraphicsLayer() const;
#endif

private:
    virtual void updateFromElement();

    virtual void intrinsicSizeChanged() { videoSizeChanged(); }

    virtual const char* renderName() const { return "RenderVideo"; }

    virtual bool requiresLayer() const { return true; }
    virtual bool isVideo() const { return true; }

    virtual void paintReplaced(PaintInfo&, int tx, int ty);

    virtual void layout();

    virtual int calcReplacedWidth(bool includeMaxWidth = true) const;
    virtual int calcReplacedHeight() const;

    virtual void calcPrefWidths();
    
    int calcAspectRatioWidth() const;
    int calcAspectRatioHeight() const;

    bool isWidthSpecified() const;
    bool isHeightSpecified() const;
    
    void updatePlayer();
};

inline RenderVideo* toRenderVideo(RenderObject* object)
{
    ASSERT(!object || object->isVideo());
    return static_cast<RenderVideo*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderVideo(const RenderVideo*);

} // namespace WebCore

#endif
#endif // RenderVideo_h
