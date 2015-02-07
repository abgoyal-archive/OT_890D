

#ifndef XSLStyleSheet_h
#define XSLStyleSheet_h

#if ENABLE(XSLT)

#include "StyleSheet.h"
#include <libxml/parser.h>
#include <libxslt/transform.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class DocLoader;
class Document;
class XSLImportRule;
    
class XSLStyleSheet : public StyleSheet {
public:
    static PassRefPtr<XSLStyleSheet> create(XSLImportRule* parentImport, const String& href)
    {
        return adoptRef(new XSLStyleSheet(parentImport, href));
    }
    static PassRefPtr<XSLStyleSheet> create(Node* parentNode, const String& href)
    {
        return adoptRef(new XSLStyleSheet(parentNode, href, false));
    }
    static PassRefPtr<XSLStyleSheet> createEmbedded(Node* parentNode, const String& href)
    {
        return adoptRef(new XSLStyleSheet(parentNode, href, true));
    }

    virtual ~XSLStyleSheet();
    
    virtual bool isXSLStyleSheet() const { return true; }

    virtual String type() const { return "text/xml"; }

    virtual bool parseString(const String &string, bool strict = true);
    
    virtual bool isLoading();
    virtual void checkLoaded();

    void loadChildSheets();
    void loadChildSheet(const String& href);

    xsltStylesheetPtr compileStyleSheet();

    DocLoader* docLoader();

    Document* ownerDocument() { return m_ownerDocument; }
    void setParentStyleSheet(XSLStyleSheet* parent);

    xmlDocPtr document();

    void clearDocuments();

    xmlDocPtr locateStylesheetSubResource(xmlDocPtr parentDoc, const xmlChar* uri);
    
    void markAsProcessed();
    bool processed() const { return m_processed; }

private:
    XSLStyleSheet(Node* parentNode, const String& href, bool embedded);
    XSLStyleSheet(XSLImportRule* parentImport, const String& href);

    Document* m_ownerDocument;
    xmlDocPtr m_stylesheetDoc;
    bool m_embedded;
    bool m_processed;
    bool m_stylesheetDocTaken;
    XSLStyleSheet* m_parentStyleSheet;
};

} // namespace WebCore

#endif // ENABLE(XSLT)

#endif // XSLStyleSheet_h
