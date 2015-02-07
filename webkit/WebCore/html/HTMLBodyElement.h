

#ifndef HTMLBodyElement_h
#define HTMLBodyElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLBodyElement : public HTMLElement {
public:
    HTMLBodyElement(const QualifiedName&, Document*);
    virtual ~HTMLBodyElement();

    String aLink() const;
    void setALink(const String&);
    String bgColor() const;
    void setBgColor(const String&);
    String link() const;
    void setLink(const String&);
    String text() const;
    void setText(const String&);
    String vLink() const;
    void setVLink(const String&);

    virtual EventListener* onblur() const;
    virtual void setOnblur(PassRefPtr<EventListener>);
    virtual EventListener* onerror() const;
    virtual void setOnerror(PassRefPtr<EventListener>);
    virtual EventListener* onfocus() const;
    virtual void setOnfocus(PassRefPtr<EventListener>);
    virtual EventListener* onload() const;
    virtual void setOnload(PassRefPtr<EventListener>);

    EventListener* onbeforeunload() const;
    void setOnbeforeunload(PassRefPtr<EventListener>);
    EventListener* onmessage() const;
    void setOnmessage(PassRefPtr<EventListener>);
    EventListener* onoffline() const;
    void setOnoffline(PassRefPtr<EventListener>);
    EventListener* ononline() const;
    void setOnonline(PassRefPtr<EventListener>);
    EventListener* onresize() const;
    void setOnresize(PassRefPtr<EventListener>);
    EventListener* onstorage() const;
    void setOnstorage(PassRefPtr<EventListener>);
    EventListener* onunload() const;
    void setOnunload(PassRefPtr<EventListener>);

private:
    virtual HTMLTagStatus endTagRequirement() const { return TagStatusRequired; }
    virtual int tagPriority() const { return 10; }
    
    virtual bool mapToEntry(const QualifiedName&, MappedAttributeEntry&) const;
    virtual void parseMappedAttribute(MappedAttribute*);

    virtual void insertedIntoDocument();

    void createLinkDecl();
    
    virtual bool isURLAttribute(Attribute*) const;

    virtual int scrollLeft() const;
    virtual void setScrollLeft(int scrollLeft);
    
    virtual int scrollTop() const;
    virtual void setScrollTop(int scrollTop);
    
    virtual int scrollHeight() const;
    virtual int scrollWidth() const;
    
    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;
    
    virtual void didMoveToNewOwnerDocument();

    RefPtr<CSSMutableStyleDeclaration> m_linkDecl;
};

} //namespace

#endif
