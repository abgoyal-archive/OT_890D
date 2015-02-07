

#ifndef SVGTextElement_h
#define SVGTextElement_h

#if ENABLE(SVG)
#include "SVGTextPositioningElement.h"
#include "SVGTransformable.h"

namespace WebCore {

    class SVGTextElement : public SVGTextPositioningElement,
                           public SVGTransformable {
    public:
        SVGTextElement(const QualifiedName&, Document*);
        virtual ~SVGTextElement();

        virtual void parseMappedAttribute(MappedAttribute*);

        virtual SVGElement* nearestViewportElement() const;
        virtual SVGElement* farthestViewportElement() const;

        virtual FloatRect getBBox() const;
        virtual TransformationMatrix getCTM() const;
        virtual TransformationMatrix getScreenCTM() const;
        virtual TransformationMatrix animatedLocalTransform() const;
        virtual TransformationMatrix* supplementalTransform();

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
        virtual bool childShouldCreateRenderer(Node*) const;
                
        virtual void svgAttributeChanged(const QualifiedName&);

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextElement, SVGNames::textTagString, SVGNames::transformAttrString, SVGTransformList, Transform, transform)
       
       // Used by <animateMotion>
       OwnPtr<TransformationMatrix> m_supplementalTransform;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
