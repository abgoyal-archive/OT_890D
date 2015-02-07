

#ifndef SVGTransformable_h
#define SVGTransformable_h

#if ENABLE(SVG)
#include "PlatformString.h"
#include "SVGLocatable.h"
#include "SVGTransformList.h"

namespace WebCore {
    
    class TransformationMatrix;
    class AtomicString;
    class SVGTransform;
    class QualifiedName;

    class SVGTransformable : virtual public SVGLocatable {
    public:
        SVGTransformable();
        virtual ~SVGTransformable();

        static bool parseTransformAttribute(SVGTransformList*, const AtomicString& transform);
        static bool parseTransformAttribute(SVGTransformList*, const UChar*& ptr, const UChar* end);
        static bool parseTransformValue(unsigned type, const UChar*& ptr, const UChar* end, SVGTransform&);
        
        TransformationMatrix getCTM(const SVGElement*) const;
        TransformationMatrix getScreenCTM(const SVGElement*) const;
        
        virtual TransformationMatrix animatedLocalTransform() const = 0;

        bool isKnownAttribute(const QualifiedName&);
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGTransformable_h
