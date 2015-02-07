

#ifndef GraphicsLayerClient_h
#define GraphicsLayerClient_h

#if USE(ACCELERATED_COMPOSITING)

namespace WebCore {

class GraphicsContext;
class GraphicsLayer;
class IntPoint;
class IntRect;
class FloatPoint;

enum GraphicsLayerPaintingPhase {
    GraphicsLayerPaintBackgroundMask = (1 << 0),
    GraphicsLayerPaintForegroundMask = (1 << 1),
    GraphicsLayerPaintAllMask = (GraphicsLayerPaintBackgroundMask | GraphicsLayerPaintForegroundMask)
};

enum AnimatedPropertyID {
    AnimatedPropertyInvalid,
    AnimatedPropertyWebkitTransform,
    AnimatedPropertyOpacity,
    AnimatedPropertyBackgroundColor
};

class GraphicsLayerClient {
public:
    virtual ~GraphicsLayerClient() {}

    // Callback for when hardware-accelerated animation started.
    virtual void notifyAnimationStarted(const GraphicsLayer*, double time) = 0;

    // Notification that a layer property changed that requires a subsequent call to syncCompositingState()
    // to appear on the screen.
    virtual void notifySyncRequired(const GraphicsLayer*) = 0;
    
    virtual void paintContents(const GraphicsLayer*, GraphicsContext&, GraphicsLayerPaintingPhase, const IntRect& inClip) = 0;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // GraphicsLayerClient_h
