

#ifndef SVGAltGlyphElement_h
#define SVGAltGlyphElement_h

#if ENABLE(SVG_FONTS)
#include "AtomicString.h"
#include "SVGTextPositioningElement.h"
#include "SVGURIReference.h"

namespace WebCore {

    class SVGGlyphElement;

    class SVGAltGlyphElement : public SVGTextPositioningElement,
                               public SVGURIReference {
    public:
        SVGAltGlyphElement(const QualifiedName&, Document*);
        virtual ~SVGAltGlyphElement();
                
        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
        bool childShouldCreateRenderer(Node*) const;

        const AtomicString& glyphRef() const;
        void setGlyphRef(const AtomicString&, ExceptionCode&);
        const AtomicString& format() const;
        void setFormat(const AtomicString&, ExceptionCode&);
    
        SVGGlyphElement* glyphElement() const;

    protected:
        virtual const SVGElement* contextElement() const { return this; }
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
