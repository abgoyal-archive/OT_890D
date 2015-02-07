

#ifndef WrapContentsInDummySpanCommand_h
#define WrapContentsInDummySpanCommand_h

#include "EditCommand.h"

namespace WebCore {

class HTMLElement;

class WrapContentsInDummySpanCommand : public SimpleEditCommand {
public:
    static PassRefPtr<WrapContentsInDummySpanCommand> create(PassRefPtr<Element> element)
    {
        return adoptRef(new WrapContentsInDummySpanCommand(element));
    }

private:
    WrapContentsInDummySpanCommand(PassRefPtr<Element>);

    virtual void doApply();
    virtual void doUnapply();

    RefPtr<Element> m_element;
    RefPtr<HTMLElement> m_dummySpan;
};

} // namespace WebCore

#endif // WrapContentsInDummySpanCommand_h
