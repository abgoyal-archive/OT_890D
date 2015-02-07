

#ifndef CharacterData_h
#define CharacterData_h

#include "Node.h"

namespace WebCore {

class CharacterData : public Node {
public:
    CharacterData(Document*, const String& text, bool isText = false);
    CharacterData(Document*, bool isText = false);
    virtual ~CharacterData();

    // DOM methods & attributes for CharacterData

    String data() const { return m_data; }
    void setData(const String&, ExceptionCode&);
    unsigned length() const { return m_data->length(); }
    String substringData(unsigned offset, unsigned count, ExceptionCode&);
    void appendData(const String&, ExceptionCode&);
    void insertData(unsigned offset, const String&, ExceptionCode&);
    void deleteData(unsigned offset, unsigned count, ExceptionCode&);
    void replaceData(unsigned offset, unsigned count, const String &arg, ExceptionCode&);

    bool containsOnlyWhitespace() const;

    // DOM methods overridden from parent classes

    virtual String nodeValue() const;
    virtual void setNodeValue(const String&, ExceptionCode&);
    
    // Other methods (not part of DOM)

    virtual bool isCharacterDataNode() const { return true; }
    virtual int maxCharacterOffset() const;
    StringImpl* string() { return m_data.get(); }

    virtual bool offsetInCharacters() const;
    virtual bool rendererIsNeeded(RenderStyle*);

protected:
    RefPtr<StringImpl> m_data;

    void dispatchModifiedEvent(StringImpl* oldValue);

private:
    void checkCharDataOperation(unsigned offset, ExceptionCode&);
};

} // namespace WebCore

#endif // CharacterData_h

