

#ifndef NamedAttrMap_h
#define NamedAttrMap_h

#include "Attribute.h"

#ifdef __OBJC__
#define id id_AVOID_KEYWORD
#endif

namespace WebCore {

class Node;

typedef int ExceptionCode;

class NamedNodeMap : public RefCounted<NamedNodeMap> {
    friend class Element;

protected:
    NamedNodeMap(Element* element) : m_element(element) { }

public:
    static PassRefPtr<NamedNodeMap> create(Element* element) { return adoptRef(new NamedNodeMap(element)); }

    virtual ~NamedNodeMap();

    // Public DOM interface.

    PassRefPtr<Node> getNamedItem(const String& name) const;
    PassRefPtr<Node> removeNamedItem(const String& name, ExceptionCode&);

    PassRefPtr<Node> getNamedItemNS(const String& namespaceURI, const String& localName) const;
    PassRefPtr<Node> removeNamedItemNS(const String& namespaceURI, const String& localName, ExceptionCode&);

    PassRefPtr<Node> getNamedItem(const QualifiedName& name) const;
    PassRefPtr<Node> removeNamedItem(const QualifiedName& name, ExceptionCode&);
    PassRefPtr<Node> setNamedItem(Node*, ExceptionCode&);
    PassRefPtr<Node> setNamedItemNS(Node* node, ExceptionCode& ec) { return setNamedItem(node, ec); }

    PassRefPtr<Node> item(unsigned index) const;
    size_t length() const { return m_attributes.size(); }
    bool isEmpty() const { return !length(); }

    // Internal interface.

    void setAttributes(const NamedNodeMap&);

    Attribute* attributeItem(unsigned index) const { return m_attributes[index].get(); }
    Attribute* getAttributeItem(const QualifiedName&) const;

    void shrinkToLength() { m_attributes.shrinkCapacity(length()); }
    void reserveInitialCapacity(unsigned capacity) { m_attributes.reserveInitialCapacity(capacity); }

    // Used during parsing: only inserts if not already there. No error checking!
    void insertAttribute(PassRefPtr<Attribute> newAttribute, bool allowDuplicates)
    {
        ASSERT(!m_element);
        if (allowDuplicates || !getAttributeItem(newAttribute->name()))
            addAttribute(newAttribute);
    }

    virtual bool isMappedAttributeMap() const;

    const AtomicString& id() const { return m_id; }
    void setID(const AtomicString& newId) { m_id = newId; }

    bool mapsEquivalent(const NamedNodeMap* otherMap) const;

    // These functions do no error checking.
    void addAttribute(PassRefPtr<Attribute>);
    void removeAttribute(const QualifiedName&);

protected:
    virtual void clearAttributes();

    Element* element() const { return m_element; }

private:
    void detachAttributesFromElement();
    void detachFromElement();
    Attribute* getAttributeItem(const String& name, bool shouldIgnoreAttributeCase) const;

    Element* m_element;
    Vector<RefPtr<Attribute> > m_attributes;
    AtomicString m_id;
};

} //namespace

#undef id

#endif
