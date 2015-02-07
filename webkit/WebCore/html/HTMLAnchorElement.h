

#ifndef HTMLAnchorElement_h
#define HTMLAnchorElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLAnchorElement : public HTMLElement {
public:
    HTMLAnchorElement(Document*);
    HTMLAnchorElement(const QualifiedName&, Document*);

    KURL href() const;
    void setHref(const AtomicString&);

    const AtomicString& name() const;

    String hash() const;
    String host() const;
    String hostname() const;
    virtual bool isFocusable() const;
    String pathname() const;
    String port() const;
    String protocol() const;
    String search() const;
    String text() const;

    String toString() const;

    bool isLiveLink() const;

protected:
    virtual void parseMappedAttribute(MappedAttribute*);

private:
    virtual HTMLTagStatus endTagRequirement() const { return TagStatusRequired; }
    virtual int tagPriority() const { return 1; }
    virtual bool supportsFocus() const;
    virtual bool isMouseFocusable() const;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const;
    virtual void defaultEventHandler(Event*);
    virtual void setActive(bool active = true, bool pause = false);
    virtual void accessKeyAction(bool fullAction);
    virtual bool isURLAttribute(Attribute*) const;
    virtual bool canStartSelection() const;
    virtual String target() const;
    virtual short tabIndex() const;
    virtual bool draggable() const;

    Element* m_rootEditableElementForSelectionOnMouseDown;
    bool m_wasShiftKeyDownOnMouseDown;
};

} // namespace WebCore

#endif // HTMLAnchorElement_h
