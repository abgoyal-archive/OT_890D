

#ifndef RenderLayerBacking_h
#define RenderLayerBacking_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"
#include "RenderLayer.h"
#include "TransformationMatrix.h"

namespace WebCore {

class KeyframeList;
class RenderLayerCompositor;

// RenderLayerBacking controls the compositing behavior for a single RenderLayer.
// It holds the various GraphicsLayers, and makes decisions about intra-layer rendering
// optimizations.
// 
// There is one RenderLayerBacking for each RenderLayer that is composited.

class RenderLayerBacking : public GraphicsLayerClient {
public:
    RenderLayerBacking(RenderLayer*);
    ~RenderLayerBacking();

    RenderLayer* owningLayer() const { return m_owningLayer; }

    enum UpdateDepth { CompositingChildren, AllDescendants };
    void updateAfterLayout(UpdateDepth);
    
    // Returns true if layer configuration changed.
    bool updateGraphicsLayerConfiguration();
    // Update graphics layer position and bounds.
    void updateGraphicsLayerGeometry(); // make private
    // Update contents and clipping structure.
    void updateInternalHierarchy(); // make private
    
    GraphicsLayer* graphicsLayer() const { return m_graphicsLayer; }

    // Layer to clip children
    bool hasClippingLayer() const { return m_clippingLayer != 0; }
    GraphicsLayer* clippingLayer() const { return m_clippingLayer; }

    // Layer to get clipped by ancestor
    bool hasAncestorClippingLayer() const { return m_ancestorClippingLayer != 0; }
    GraphicsLayer* ancestorClippingLayer() const { return m_ancestorClippingLayer; }

    bool hasContentsLayer() const { return m_foregroundLayer != 0; }
    GraphicsLayer* foregroundLayer() const { return m_foregroundLayer; }
    
    GraphicsLayer* parentForSublayers() const { return m_clippingLayer ? m_clippingLayer : m_graphicsLayer; }
    GraphicsLayer* childForSuperlayers() const { return m_ancestorClippingLayer ? m_ancestorClippingLayer : m_graphicsLayer; }

    // RenderLayers with backing normally short-circuit paintLayer() because
    // their content is rendered via callbacks from GraphicsLayer. However, the document
    // layer is special, because it has a GraphicsLayer to act as a container for the GraphicsLayers
    // for descendants, but its contents usually render into the window (in which case this returns true).
    // This returns false for other layers, and when the document layer actually needs to paint into its backing store
    // for some reason.
    bool paintingGoesToWindow() const;

    void setContentsNeedDisplay();
    // r is in the coordinate space of the layer's render object
    void setContentsNeedDisplayInRect(const IntRect& r);

    // Notification from the renderer that its content changed; used by RenderImage.
    void rendererContentChanged();

    // Interface to start, finish, suspend and resume animations and transitions
    bool startAnimation(double beginTime, const Animation* anim, const KeyframeList& keyframes);
    bool startTransition(double beginTime, int property, const RenderStyle* fromStyle, const RenderStyle* toStyle);
    void animationFinished(const String& name);
    void animationPaused(const String& name);
    void transitionFinished(int property);

    void suspendAnimations(double time = 0);
    void resumeAnimations();

    IntRect compositedBounds() const;
    void setCompositedBounds(const IntRect&);

    FloatPoint graphicsLayerToContentsCoordinates(const GraphicsLayer*, const FloatPoint&);
    FloatPoint contentsToGraphicsLayerCoordinates(const GraphicsLayer*, const FloatPoint&);

    // GraphicsLayerClient interface
    virtual void notifyAnimationStarted(const GraphicsLayer*, double startTime);
    virtual void notifySyncRequired(const GraphicsLayer*);

    virtual void paintContents(const GraphicsLayer*, GraphicsContext&, GraphicsLayerPaintingPhase, const IntRect& clip);

    IntRect contentsBox() const;
    
private:
    void createGraphicsLayer();
    void destroyGraphicsLayer();

    RenderBoxModelObject* renderer() const { return m_owningLayer->renderer(); }
    RenderLayerCompositor* compositor() const { return m_owningLayer->compositor(); }

    bool updateClippingLayers(bool needsAncestorClip, bool needsDescendantClip);
    bool updateForegroundLayer(bool needsForegroundLayer);

    IntSize contentOffsetInCompostingLayer() const;
    // Result is transform origin in pixels.
    FloatPoint3D computeTransformOrigin(const IntRect& borderBox) const;
    // Result is perspective origin in pixels.
    FloatPoint computePerspectiveOrigin(const IntRect& borderBox) const;

    void updateLayerOpacity();
    void updateLayerTransform();

    // Return the opacity value that this layer should use for compositing.
    float compositingOpacity(float rendererOpacity) const;
    
    // Returns true if this RenderLayer only has content that can be rendered directly
    // by the compositing layer, without drawing (e.g. solid background color).
    bool isSimpleContainerCompositingLayer() const;
    // Returns true if we can optimize the RenderLayer to draw the replaced content
    // directly into a compositing buffer
    bool canUseDirectCompositing() const;
    void updateImageContents();

    bool rendererHasBackground() const;
    const Color& rendererBackgroundColor() const;

    bool hasNonCompositingContent() const;
    
    void paintIntoLayer(RenderLayer* rootLayer, GraphicsContext*, const IntRect& paintDirtyRect,
                    PaintRestriction paintRestriction, GraphicsLayerPaintingPhase, RenderObject* paintingRoot);

    static int graphicsLayerToCSSProperty(AnimatedPropertyID);
    static AnimatedPropertyID cssToGraphicsLayerProperty(int);

private:
    RenderLayer* m_owningLayer;

    GraphicsLayer* m_ancestorClippingLayer; // only used if we are clipped by an ancestor which is not a stacking context
    GraphicsLayer* m_graphicsLayer;
    GraphicsLayer* m_foregroundLayer;       // only used in cases where we need to draw the foreground separately
    GraphicsLayer* m_clippingLayer;         // only used if we have clipping on a stacking context, with compositing children

    IntRect m_compositedBounds;

    bool m_hasDirectlyCompositedContent;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // RenderLayerBacking_h
