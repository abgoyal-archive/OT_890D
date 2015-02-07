

#ifndef SVGTextContentElement_h
#define SVGTextContentElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledElement.h"
#include "SVGTests.h"

namespace WebCore {

    extern char SVGTextContentElementIdentifier[];

    class SVGLength;

    class SVGTextContentElement : public SVGStyledElement,
                                  public SVGTests,
                                  public SVGLangSpace,
                                  public SVGExternalResourcesRequired {
    public:
        enum SVGLengthAdjustType {
            LENGTHADJUST_UNKNOWN            = 0,
            LENGTHADJUST_SPACING            = 1,
            LENGTHADJUST_SPACINGANDGLYPHS   = 2
        };

        SVGTextContentElement(const QualifiedName&, Document*);
        virtual ~SVGTextContentElement();
        
        virtual bool isValid() const { return SVGTests::isValid(); }
        virtual bool isTextContent() const { return true; }

        unsigned getNumberOfChars() const;
        float getComputedTextLength() const;
        float getSubStringLength(unsigned charnum, unsigned nchars, ExceptionCode&) const;
        FloatPoint getStartPositionOfChar(unsigned charnum, ExceptionCode&) const;
        FloatPoint getEndPositionOfChar(unsigned charnum, ExceptionCode&) const;
        FloatRect getExtentOfChar(unsigned charnum, ExceptionCode&) const;
        float getRotationOfChar(unsigned charnum, ExceptionCode&) const;
        int getCharNumAtPosition(const FloatPoint&) const;
        void selectSubString(unsigned charnum, unsigned nchars, ExceptionCode&) const;

        virtual void parseMappedAttribute(MappedAttribute*);

        bool isKnownAttribute(const QualifiedName&);

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextContentElement, SVGTextContentElementIdentifier, SVGNames::textLengthAttrString, SVGLength, TextLength, textLength)
        ANIMATED_PROPERTY_DECLARATIONS(SVGTextContentElement, SVGTextContentElementIdentifier, SVGNames::lengthAdjustAttrString, int, LengthAdjust, lengthAdjust)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
