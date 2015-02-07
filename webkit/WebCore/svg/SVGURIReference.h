

#ifndef SVGURIReference_h
#define SVGURIReference_h

#if ENABLE(SVG)
#include "SVGElement.h"
#include "XLinkNames.h"

namespace WebCore {

    extern char SVGURIReferenceIdentifier[];
    class MappedAttribute;

    class SVGURIReference {
    public:
        SVGURIReference();
        virtual ~SVGURIReference();

        bool parseMappedAttribute(MappedAttribute*);
        bool isKnownAttribute(const QualifiedName&);

        static String getTarget(const String& url);

        virtual const SVGElement* contextElement() const = 0;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGURIReference, SVGURIReferenceIdentifier, XLinkNames::hrefAttrString, String, Href, href)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGURIReference_h
