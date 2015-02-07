

#ifndef HTMLFrameElement_h
#define HTMLFrameElement_h

#include "HTMLFrameElementBase.h"

namespace WebCore {

class Document;
class RenderObject;
class RenderArena;
class RenderStyle;

class HTMLFrameElement : public HTMLFrameElementBase {
public:
    HTMLFrameElement(const QualifiedName&, Document*);

    virtual HTMLTagStatus endTagRequirement() const { return TagStatusForbidden; }
    virtual int tagPriority() const { return 0; }
  
    virtual void attach();

    virtual bool rendererIsNeeded(RenderStyle*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    
    virtual void parseMappedAttribute(MappedAttribute*);

    bool hasFrameBorder() const { return m_frameBorder; }

private:
    bool m_frameBorder;
    bool m_frameBorderSet;
};

} // namespace WebCore

#endif // HTMLFrameElement_h
