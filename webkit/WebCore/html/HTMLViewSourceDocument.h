

#ifndef HTMLViewSourceDocument_h
#define HTMLViewSourceDocument_h

#include "HTMLDocument.h"

namespace WebCore {

class DoctypeToken;
struct Token;

class HTMLViewSourceDocument : public HTMLDocument {
public:
    static PassRefPtr<HTMLViewSourceDocument> create(Frame* frame, const String& mimeType)
    {
        return new HTMLViewSourceDocument(frame, mimeType);
    }

    // Returns HTMLTokenizer or TextTokenizer based on m_type.
    virtual Tokenizer* createTokenizer();

    void addViewSourceToken(Token*); // Used by the HTML tokenizer.
    void addViewSourceText(const String&); // Used by the plaintext tokenizer.
    void addViewSourceDoctypeToken(DoctypeToken*);

private:
    HTMLViewSourceDocument(Frame*, const String& mimeType);

    void createContainingTable();
    Element* addSpanWithClassName(const String&);
    void addLine(const String& className);
    void addText(const String& text, const String& className);
    Element* addLink(const String& url, bool isAnchor);

    String m_type;
    Element* m_current;
    Element* m_tbody;
    Element* m_td;
};

}

#endif // HTMLViewSourceDocument_h
