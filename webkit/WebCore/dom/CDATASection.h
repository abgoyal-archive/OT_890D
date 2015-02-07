

#ifndef CDATASection_h
#define CDATASection_h

#include "Text.h"

namespace WebCore {

class CDATASection : public Text {
public:
    CDATASection(Document*, const String&);
    virtual ~CDATASection();

    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual PassRefPtr<Node> cloneNode(bool deep);
    virtual bool childTypeAllowed(NodeType);

protected:
    virtual PassRefPtr<Text> createNew(PassRefPtr<StringImpl>);
};

} // namespace WebCore

#endif // CDATASection_h
