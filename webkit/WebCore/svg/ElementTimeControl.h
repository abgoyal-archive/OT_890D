

#ifndef ElementTimeControl_h
#define ElementTimeControl_h

#if ENABLE(SVG)

#include "ExceptionCode.h"

namespace WebCore {

    class ElementTimeControl {
    public:
        virtual ~ElementTimeControl() {}
        virtual bool beginElement(ExceptionCode&) = 0;
        virtual bool beginElementAt(float offset, ExceptionCode&) = 0;
        virtual bool endElement(ExceptionCode&) = 0;
        virtual bool endElementAt(float offset, ExceptionCode&) = 0;
    };
        
}

#endif

#endif
