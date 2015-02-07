

#ifndef Notation_h
#define Notation_h

#include "CachedResourceClient.h"
#include "ContainerNode.h"

namespace WebCore {

class Notation : public ContainerNode {
public:
    Notation(Document*);
    Notation(Document*, const String& name, const String& publicId, const String& systemId);

    // DOM methods & attributes for Notation
    String publicId() const { return m_publicId; }
    String systemId() const { return m_systemId; }

    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual PassRefPtr<Node> cloneNode(bool deep);
    virtual bool childTypeAllowed(NodeType);

private:
    String m_name;
    String m_publicId;
    String m_systemId;
};

} //namespace

#endif
