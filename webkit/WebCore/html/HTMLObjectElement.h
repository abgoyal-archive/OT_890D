

#ifndef HTMLObjectElement_h
#define HTMLObjectElement_h

#include "HTMLPlugInImageElement.h"

namespace WebCore {

class KURL;

class HTMLObjectElement : public HTMLPlugInImageElement {
public:
    HTMLObjectElement(const QualifiedName&, Document*, bool createdByParser);
    ~HTMLObjectElement();

    virtual int tagPriority() const { return 5; }

    virtual void parseMappedAttribute(MappedAttribute*);

    virtual void attach();
    virtual bool canLazyAttach() { return false; }
    virtual bool rendererIsNeeded(RenderStyle*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void finishParsingChildren();
    virtual void detach();
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    
    virtual void recalcStyle(StyleChange);
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual bool isURLAttribute(Attribute*) const;
    virtual const QualifiedName& imageSourceAttributeName() const;

    virtual void updateWidget();
    void setNeedWidgetUpdate(bool needWidgetUpdate) { m_needWidgetUpdate = needWidgetUpdate; }

    void renderFallbackContent();

    virtual RenderWidget* renderWidgetForJSBindings() const;

    String archive() const;
    void setArchive(const String&);

    String border() const;
    void setBorder(const String&);

    String code() const;
    void setCode(const String&);
    
    String codeBase() const;
    void setCodeBase(const String&);

    String codeType() const;
    void setCodeType(const String&);
    
    KURL data() const;
    void setData(const String&);

    bool declare() const;
    void setDeclare(bool);

    int hspace() const;
    void setHspace(int);

    String standby() const;
    void setStandby(const String&);

    String type() const;
    void setType(const String&);

    String useMap() const;
    void setUseMap(const String&);

    int vspace() const;
    void setVspace(int);

    bool isDocNamedItem() const { return m_docNamedItem; }

    const String& classId() const { return m_classId; }

    bool containsJavaApplet() const;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

private:
    void updateDocNamedItem();

    AtomicString m_id;
    String m_classId;
    bool m_docNamedItem : 1;
    bool m_needWidgetUpdate : 1;
    bool m_useFallbackContent : 1;
};

}

#endif
