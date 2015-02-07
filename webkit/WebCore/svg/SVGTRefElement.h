

#ifndef SVGTRefElement_h
#define SVGTRefElement_h

#if ENABLE(SVG)
#include "SVGTextPositioningElement.h"
#include "SVGURIReference.h"

namespace WebCore {

    class SVGTRefElement : public SVGTextPositioningElement,
                           public SVGURIReference {
    public:
        SVGTRefElement(const QualifiedName&, Document*);
        virtual ~SVGTRefElement();

        virtual void parseMappedAttribute(MappedAttribute*);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
        bool childShouldCreateRenderer(Node*) const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        void updateReferencedText();
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
