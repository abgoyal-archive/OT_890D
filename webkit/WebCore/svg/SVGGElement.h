

#ifndef SVGGElement_h
#define SVGGElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

    class SVGGElement : public SVGStyledTransformableElement,
                        public SVGTests,
                        public SVGLangSpace,
                        public SVGExternalResourcesRequired {
    public:
        SVGGElement(const QualifiedName&, Document*);
        virtual ~SVGGElement();

        virtual bool isValid() const { return SVGTests::isValid(); }

        virtual void parseMappedAttribute(MappedAttribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        friend class SVGUseElement;
        TransformationMatrix localMatrix() const;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
