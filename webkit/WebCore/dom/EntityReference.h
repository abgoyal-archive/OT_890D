

#ifndef EntityReference_h
#define EntityReference_h

#include "ContainerNode.h"

namespace WebCore {

class EntityReference : public ContainerNode {
public:
    EntityReference(Document*, const String& entityName);

    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual PassRefPtr<Node> cloneNode(bool deep);

private:
    String m_entityName;
};

} //namespace

#endif
