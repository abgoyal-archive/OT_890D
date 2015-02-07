

#ifndef SVGFEDisplacementMapElement_h
#define SVGFEDisplacementMapElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEDisplacementMap.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    
    class SVGFEDisplacementMapElement : public SVGFilterPrimitiveStandardAttributes {
    public:
        SVGFEDisplacementMapElement(const QualifiedName& tagName, Document*);
        virtual ~SVGFEDisplacementMapElement();
        
        static ChannelSelectorType stringToChannel(const String&);
        
        virtual void parseMappedAttribute(MappedAttribute*);
        virtual bool build(SVGResourceFilter*);
        
    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDisplacementMapElement, SVGNames::feDisplacementMapTagString, SVGNames::inAttrString, String, In1, in1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDisplacementMapElement, SVGNames::feDisplacementMapTagString, SVGNames::in2AttrString, String, In2, in2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDisplacementMapElement, SVGNames::feDisplacementMapTagString, SVGNames::xChannelSelectorAttrString, int, XChannelSelector, xChannelSelector)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDisplacementMapElement, SVGNames::feDisplacementMapTagString, SVGNames::yChannelSelectorAttrString, int, YChannelSelector, yChannelSelector)
        ANIMATED_PROPERTY_DECLARATIONS(SVGFEDisplacementMapElement, SVGNames::feDisplacementMapTagString, SVGNames::scaleAttrString, float, Scale, scale)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGFEDisplacementMapElement_h
