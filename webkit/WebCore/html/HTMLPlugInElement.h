

#ifndef HTMLPlugInElement_h
#define HTMLPlugInElement_h

#include "HTMLFrameOwnerElement.h"
#include "ScriptInstance.h"

#if ENABLE(NETSCAPE_PLUGIN_API)
struct NPObject;
#endif

namespace WebCore {

class RenderWidget;

class HTMLPlugInElement : public HTMLFrameOwnerElement {
public:
    HTMLPlugInElement(const QualifiedName& tagName, Document*);
    virtual ~HTMLPlugInElement();

    virtual bool mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const;
    virtual void parseMappedAttribute(MappedAttribute*);

    virtual HTMLTagStatus endTagRequirement() const { return TagStatusRequired; }
    virtual bool checkDTD(const Node* newChild);

    virtual void updateWidget() { }

    String align() const;
    void setAlign(const String&);
    
    String height() const;
    void setHeight(const String&);
    
    String name() const;
    void setName(const String&);
    
    String width() const;
    void setWidth(const String&);

    virtual void defaultEventHandler(Event*);

    virtual RenderWidget* renderWidgetForJSBindings() const = 0;
    virtual void detach();
    PassScriptInstance getInstance() const;

#if ENABLE(NETSCAPE_PLUGIN_API)
    virtual NPObject* getNPObject();
#endif

protected:
    static void updateWidgetCallback(Node*);

    AtomicString m_name;
    mutable ScriptInstance m_instance;
#if ENABLE(NETSCAPE_PLUGIN_API)
    NPObject* m_NPObject;
#endif
};

} // namespace WebCore

#endif // HTMLPlugInElement_h
