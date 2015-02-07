

#ifndef SVGExternalResourcesRequired_h
#define SVGExternalResourcesRequired_h

#if ENABLE(SVG)
#include "SVGElement.h"

namespace WebCore {

    extern char SVGExternalResourcesRequiredIdentifier[];
    class MappedAttribute;

    // FIXME: This is wrong for several reasons:
    // 1. externalResourcesRequired is not animateable according to SVG 1.1 section 5.9
    // 2. externalResourcesRequired should just be part of SVGElement, and default to "false" for all elements
    /*
     SPEC: Note that the SVG DOM 
     defines the attribute externalResourcesRequired as being of type SVGAnimatedBoolean, whereas the 
     SVG language definition says that externalResourcesRequired is not animated. Because the SVG 
     language definition states that externalResourcesRequired cannot be animated, the animVal will 
     always be the same as the baseVal.
     */
    class SVGExternalResourcesRequired {
    public:
        SVGExternalResourcesRequired();
        virtual ~SVGExternalResourcesRequired();

        bool parseMappedAttribute(MappedAttribute*);
        bool isKnownAttribute(const QualifiedName&);

        virtual const SVGElement* contextElement() const = 0;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGExternalResourcesRequired, SVGExternalResourcesRequiredIdentifier,
                                       SVGNames::externalResourcesRequiredAttrString, bool,
                                       ExternalResourcesRequired, externalResourcesRequired)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
