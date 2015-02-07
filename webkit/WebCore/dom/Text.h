

#ifndef Text_h
#define Text_h

#include "CharacterData.h"

namespace WebCore {
    
const unsigned cTextNodeLengthLimit = 1 << 16;

class Text : public CharacterData {
public:
    Text(Document *impl, const String &_text);
    Text(Document *impl);
    virtual ~Text();

    // DOM methods & attributes for CharacterData

    PassRefPtr<Text> splitText(unsigned offset, ExceptionCode&);

    // DOM Level 3: http://www.w3.org/TR/DOM-Level-3-Core/core.html#ID-1312295772
    String wholeText() const;
    PassRefPtr<Text> replaceWholeText(const String&, ExceptionCode&);

    // DOM methods overridden from parent classes

    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual PassRefPtr<Node> cloneNode(bool deep);

    // Other methods (not part of DOM)

    virtual void attach();
    virtual bool rendererIsNeeded(RenderStyle*);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void recalcStyle(StyleChange = NoChange);
    virtual bool childTypeAllowed(NodeType);

    static PassRefPtr<Text> createWithLengthLimit(Document*, const String&, unsigned& charsLeft, unsigned maxChars = cTextNodeLengthLimit);

#ifndef NDEBUG
    virtual void formatForDebugger(char* buffer, unsigned length) const;
#endif

protected:
    virtual PassRefPtr<Text> createNew(PassRefPtr<StringImpl>);
};

} // namespace WebCore

#endif // Text_h
