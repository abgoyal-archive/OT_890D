

#ifndef SVGStyledTransformableElement_h
#define SVGStyledTransformableElement_h

#if ENABLE(SVG)
#include "Path.h"
#include "SVGStyledLocatableElement.h"
#include "SVGTransformable.h"

namespace WebCore {

    extern char SVGStyledTransformableElementIdentifier[];

    class TransformationMatrix;

    class SVGStyledTransformableElement : public SVGStyledLocatableElement,
                                          public SVGTransformable {
    public:
        SVGStyledTransformableElement(const QualifiedName&, Document*);
        virtual ~SVGStyledTransformableElement();
        
        virtual bool isStyledTransformable() const { return true; }

        virtual TransformationMatrix getCTM() const;
        virtual TransformationMatrix getScreenCTM() const;
        virtual SVGElement* nearestViewportElement() const;
        virtual SVGElement* farthestViewportElement() const;
        
        virtual TransformationMatrix animatedLocalTransform() const;
        virtual TransformationMatrix* supplementalTransform();

        virtual FloatRect getBBox() const;

        virtual void parseMappedAttribute(MappedAttribute*);
        bool isKnownAttribute(const QualifiedName&);

        // "base class" methods for all the elements which render as paths
        virtual Path toPathData() const { return Path(); }
        virtual Path toClipPath() const;
        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    protected:
        ANIMATED_PROPERTY_DECLARATIONS(SVGStyledTransformableElement, SVGStyledTransformableElementIdentifier,
                                       SVGNames::transformAttrString, SVGTransformList, Transform, transform)

    private:
        // Used by <animateMotion>
        OwnPtr<TransformationMatrix> m_supplementalTransform;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGStyledTransformableElement_h
