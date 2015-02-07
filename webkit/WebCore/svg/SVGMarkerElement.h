

#ifndef SVGMarkerElement_h
#define SVGMarkerElement_h

#if ENABLE(SVG)

#include "SVGAngle.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGLangSpace.h"
#include "SVGResourceMarker.h"
#include "SVGStyledElement.h"

namespace WebCore {

    class Document;
    class SVGAngle;

    extern char SVGOrientTypeAttrIdentifier[];
    extern char SVGOrientAngleAttrIdentifier[];

    class SVGMarkerElement : public SVGStyledElement,
                             public SVGLangSpace,
                             public SVGExternalResourcesRequired,
                             public SVGFitToViewBox {
    public:
        enum SVGMarkerUnitsType {
            SVG_MARKERUNITS_UNKNOWN           = 0,
            SVG_MARKERUNITS_USERSPACEONUSE    = 1,
            SVG_MARKERUNITS_STROKEWIDTH       = 2
        };

        enum SVGMarkerOrientType {
            SVG_MARKER_ORIENT_UNKNOWN    = 0,
            SVG_MARKER_ORIENT_AUTO       = 1,
            SVG_MARKER_ORIENT_ANGLE      = 2
        };

        SVGMarkerElement(const QualifiedName&, Document*);
        virtual ~SVGMarkerElement();

        void setOrientToAuto();
        void setOrientToAngle(PassRefPtr<SVGAngle>);

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
        virtual SVGResource* canvasResource();

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGNames::refXAttrString, SVGLength, RefX, refX)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGNames::refYAttrString, SVGLength, RefY, refY)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGNames::markerWidthAttrString, SVGLength, MarkerWidth, markerWidth)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGNames::markerHeightAttrString, SVGLength, MarkerHeight, markerHeight)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGNames::markerUnitsAttrString, int, MarkerUnits, markerUnits)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGOrientTypeAttrIdentifier, int, OrientType, orientType)
        ANIMATED_PROPERTY_DECLARATIONS(SVGMarkerElement, SVGNames::markerTagString, SVGOrientAngleAttrIdentifier, SVGAngle, OrientAngle, orientAngle)

        RefPtr<SVGResourceMarker> m_marker;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
