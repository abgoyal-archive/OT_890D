

#ifndef SVGResourceMarker_h
#define SVGResourceMarker_h

#if ENABLE(SVG)

#include "FloatRect.h"
#include "SVGResource.h"

namespace WebCore {

    class GraphicsContext;
    class RenderSVGViewportContainer;

    class SVGResourceMarker : public SVGResource {
    public:
        static PassRefPtr<SVGResourceMarker> create() { return adoptRef(new SVGResourceMarker); }
        virtual ~SVGResourceMarker();

        void setMarker(RenderSVGViewportContainer*);

        void setRef(double refX, double refY);
        double refX() const { return m_refX; }
        double refY() const { return m_refY; }

        void setAngle(float angle) { m_angle = angle; }
        void setAutoAngle() { m_angle = -1; }
        float angle() const { return m_angle; }

        void setUseStrokeWidth(bool useStrokeWidth = true) { m_useStrokeWidth = useStrokeWidth; }
        bool useStrokeWidth() const { return m_useStrokeWidth; }

        FloatRect cachedBounds() const;
        void draw(GraphicsContext*, const FloatRect&, double x, double y, double strokeWidth = 1, double angle = 0);
        
        virtual SVGResourceType resourceType() const { return MarkerResourceType; }
        virtual TextStream& externalRepresentation(TextStream&) const;

    private:
        SVGResourceMarker();
        double m_refX, m_refY;
        FloatRect m_cachedBounds;
        float m_angle;
        RenderSVGViewportContainer* m_marker;
        bool m_useStrokeWidth;
    };

    SVGResourceMarker* getMarkerById(Document*, const AtomicString&);

} // namespace WebCore

#endif

#endif // SVGResourceMarker_h
