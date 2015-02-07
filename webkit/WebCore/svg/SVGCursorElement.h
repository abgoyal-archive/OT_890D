

#ifndef SVGCursorElement_h
#define SVGCursorElement_h

#if ENABLE(SVG)
#include "SVGLength.h"
#include "SVGElement.h"
#include "SVGTests.h"
#include "SVGURIReference.h"
#include "SVGExternalResourcesRequired.h"

namespace WebCore {

    class SVGCursorElement : public SVGElement,
                             public SVGTests,
                             public SVGExternalResourcesRequired,
                             public SVGURIReference {
    public:
        SVGCursorElement(const QualifiedName&, Document*);
        virtual ~SVGCursorElement();

        void addClient(SVGElement*);
        void removeClient(SVGElement*);

        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);

        virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGCursorElement, SVGNames::cursorTagString, SVGNames::xAttrString, SVGLength, X, x)
        ANIMATED_PROPERTY_DECLARATIONS(SVGCursorElement, SVGNames::cursorTagString, SVGNames::yAttrString, SVGLength, Y, y)

        HashSet<SVGElement*> m_clients;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
