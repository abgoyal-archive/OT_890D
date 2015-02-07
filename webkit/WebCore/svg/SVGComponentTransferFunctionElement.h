

#ifndef SVGComponentTransferFunctionElement_h
#define SVGComponentTransferFunctionElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGElement.h"
#include "SVGNumberList.h"
#include "FEComponentTransfer.h"

namespace WebCore {

    extern char SVGComponentTransferFunctionElementIdentifier[];

    class SVGComponentTransferFunctionElement : public SVGElement {
    public:
        SVGComponentTransferFunctionElement(const QualifiedName&, Document*);
        virtual ~SVGComponentTransferFunctionElement();

        virtual void parseMappedAttribute(MappedAttribute* attr);
        
        ComponentTransferFunction transferFunction() const;

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::typeAttrString, int, Type, type)
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::tableValuesAttrString, SVGNumberList, TableValues, tableValues)
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::slopeAttrString, float, Slope, slope)
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::interceptAttrString, float, Intercept, intercept)
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::amplitudeAttrString, float, Amplitude, amplitude)
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::exponentAttrString, float, Exponent, exponent)
        ANIMATED_PROPERTY_DECLARATIONS(SVGComponentTransferFunctionElement, SVGComponentTransferFunctionElementIdentifier, SVGNames::offsetAttrString, float, Offset, offset)
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
#endif
