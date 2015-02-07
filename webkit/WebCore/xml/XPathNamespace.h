

#ifndef XPathNamespace_h
#define XPathNamespace_h

#if ENABLE(XPATH)

#include "AtomicString.h"
#include "Node.h"

namespace WebCore {

    class Document;
    class Element;

    class XPathNamespace : public Node {
    public:
        XPathNamespace(PassRefPtr<Element> ownerElement, const String& prefix, const String& uri);
        virtual ~XPathNamespace();

        virtual Document* ownerDocument() const;
        virtual Element* ownerElement() const;

        virtual const AtomicString& prefix() const;
        virtual String nodeName() const;
        virtual String nodeValue() const;
        virtual const AtomicString& namespaceURI() const;

        virtual Node::NodeType nodeType() const;

    private:
        RefPtr<Element> m_ownerElement;
        AtomicString m_prefix;
        AtomicString m_uri;
    };

}

#endif // ENABLE(XPATH)

#endif // XPathNamespace_h

