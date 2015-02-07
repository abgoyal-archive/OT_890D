

#ifndef SVGSymbolElement_h
#define SVGSymbolElement_h
#if ENABLE(SVG)

#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGLangSpace.h"
#include "SVGStyledElement.h"

namespace WebCore {

    class SVGSymbolElement : public SVGStyledElement,
                             public SVGLangSpace,
                             public SVGExternalResourcesRequired,
                             public SVGFitToViewBox {
    public:
        SVGSymbolElement(const QualifiedName&, Document*);
        virtual ~SVGSymbolElement();

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool shouldAttachChild(Element*) const { return false; }
    
        virtual bool rendererIsNeeded(RenderStyle*) { return false; }

    protected:
        virtual const SVGElement* contextElement() const { return this; }
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
