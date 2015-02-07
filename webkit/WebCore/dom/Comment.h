

#ifndef Comment_h
#define Comment_h

#include "CharacterData.h"

namespace WebCore {

class Comment : public CharacterData {
public:
    Comment(Document*, const String &_text);
    Comment(Document*);
    virtual ~Comment();

    // DOM methods overridden from  parent classes
    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual PassRefPtr<Node> cloneNode(bool deep);

    // Other methods (not part of DOM)
    virtual bool isCommentNode() const { return true; }
    virtual bool childTypeAllowed(NodeType);
};

} // namespace WebCore

#endif // Comment_h
