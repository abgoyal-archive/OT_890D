

#ifndef HTMLEmbedElement_h
#define HTMLEmbedElement_h

#include "HTMLPlugInImageElement.h"

namespace WebCore {

class HTMLEmbedElement : public HTMLPlugInImageElement {
public:
    HTMLEmbedElement(const QualifiedName&, Document*);
    ~HTMLEmbedElement();

    virtual HTMLTagStatus endTagRequirement() const { return TagStatusForbidden; }
    virtual int tagPriority() const { return 0; }

    virtual bool mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const;
    virtual void parseMappedAttribute(MappedAttribute*);

    virtual void attach();
    virtual bool canLazyAttach() { return false; }
    virtual bool rendererIsNeeded(RenderStyle*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void attributeChanged(Attribute*, bool preserveDecls = false);
    
    virtual bool isURLAttribute(Attribute*) const;
    virtual const QualifiedName& imageSourceAttributeName() const;

    virtual void updateWidget();
    void setNeedWidgetUpdate(bool needWidgetUpdate) { m_needWidgetUpdate = needWidgetUpdate; }

    virtual RenderWidget* renderWidgetForJSBindings() const;

    String src() const;
    void setSrc(const String&);

    String type() const;
    void setType(const String&);

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

private:
    bool m_needWidgetUpdate;
};

}

#endif
