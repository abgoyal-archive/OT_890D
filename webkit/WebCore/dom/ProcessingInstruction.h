

#ifndef ProcessingInstruction_h
#define ProcessingInstruction_h

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "ContainerNode.h"

namespace WebCore {

class StyleSheet;
class CSSStyleSheet;

class ProcessingInstruction : public ContainerNode, private CachedResourceClient {
public:
    ProcessingInstruction(Document*);
    ProcessingInstruction(Document*, const String& target, const String& data);
    virtual ~ProcessingInstruction();

    // DOM methods & attributes for Notation
    String target() const { return m_target; }
    String data() const { return m_data; }
    void setData(const String&, ExceptionCode&);

    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual String nodeValue() const;
    virtual void setNodeValue(const String&, ExceptionCode&);
    virtual PassRefPtr<Node> cloneNode(bool deep);
    virtual bool childTypeAllowed(NodeType);
    virtual bool offsetInCharacters() const;
    virtual int maxCharacterOffset() const;

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    void setCreatedByParser(bool createdByParser) { m_createdByParser = createdByParser; }
    virtual void finishParsingChildren();

    // Other methods (not part of DOM)
    String localHref() const { return m_localHref; }
    StyleSheet* sheet() const { return m_sheet.get(); }
    void checkStyleSheet();
    virtual void setCSSStyleSheet(const String& url, const String& charset, const CachedCSSStyleSheet*);
#if ENABLE(XSLT)
    virtual void setXSLStyleSheet(const String& url, const String& sheet);
#endif
    void setCSSStyleSheet(PassRefPtr<CSSStyleSheet>);
    bool isLoading() const;
    virtual bool sheetLoaded();

#if ENABLE(XSLT)
    bool isXSL() const { return m_isXSL; }
#endif

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

private:
    void parseStyleSheet(const String& sheet);

    String m_target;
    String m_data;
    String m_localHref;
    String m_title;
    String m_media;
    CachedResourceHandle<CachedResource> m_cachedSheet;
    RefPtr<StyleSheet> m_sheet;
    bool m_loading;
    bool m_alternate;
    bool m_createdByParser;
#if ENABLE(XSLT)
    bool m_isXSL;
#endif
};

} //namespace

#endif
