

#ifndef HTMLAppletElement_h
#define HTMLAppletElement_h

#include "HTMLPlugInElement.h"

namespace WebCore {

class HTMLFormElement;
class HTMLImageLoader;

class HTMLAppletElement : public HTMLPlugInElement {
public:
    HTMLAppletElement(const QualifiedName&, Document*);

    String hspace() const;
    void setHspace(const String&);

    String vspace() const;
    void setVspace(const String&);

private:
    virtual int tagPriority() const { return 1; }

    virtual void parseMappedAttribute(MappedAttribute*);
    
    virtual bool rendererIsNeeded(RenderStyle*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void finishParsingChildren();
    
    virtual RenderWidget* renderWidgetForJSBindings() const;

    void setupApplet() const;

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();

    AtomicString m_id;
};

}

#endif
