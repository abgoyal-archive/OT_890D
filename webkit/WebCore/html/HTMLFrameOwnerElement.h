

#ifndef HTMLFrameOwnerElement_h
#define HTMLFrameOwnerElement_h

#include "HTMLElement.h"

namespace WebCore {

class DOMWindow;
class Frame;
class KeyboardEvent;

#if ENABLE(SVG)
class SVGDocument;
#endif

class HTMLFrameOwnerElement : public HTMLElement {
protected:
    HTMLFrameOwnerElement(const QualifiedName& tagName, Document*);

public:
    virtual ~HTMLFrameOwnerElement();

    virtual void willRemove();

    Frame* contentFrame() const { return m_contentFrame; }
    DOMWindow* contentWindow() const;
    Document* contentDocument() const;

    virtual bool isFrameOwnerElement() const { return true; }
    virtual bool isKeyboardFocusable(KeyboardEvent*) const { return m_contentFrame; }
    
    virtual ScrollbarMode scrollingMode() const { return ScrollbarAuto; }

#if ENABLE(SVG)
    SVGDocument* getSVGDocument(ExceptionCode&) const;
#endif

private:
    friend class Frame;
    Frame* m_contentFrame;
};

} // namespace WebCore

#endif // HTMLFrameOwnerElement_h
