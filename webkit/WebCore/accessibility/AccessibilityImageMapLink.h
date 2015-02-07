

#ifndef AccessibilityImageMapLink_h
#define AccessibilityImageMapLink_h

#include "AccessibilityObject.h"
#include "HTMLAreaElement.h"
#include "HTMLMapElement.h"

namespace WebCore {
    
class AccessibilityImageMapLink : public AccessibilityObject {
        
private:
    AccessibilityImageMapLink();
public:
    static PassRefPtr<AccessibilityImageMapLink> create();
    virtual ~AccessibilityImageMapLink();
    
    void setHTMLAreaElement(HTMLAreaElement* element) { m_areaElement = element; }
    void setHTMLMapElement(HTMLMapElement* element) { m_mapElement = element; }    
    void setParent(AccessibilityObject* parent) { m_parent = parent; }
        
    virtual AccessibilityRole roleValue() const { return WebCoreLinkRole; }
    virtual bool accessibilityIsIgnored() const { return false; }
    virtual bool isEnabled() const { return true; }

    virtual AccessibilityObject* parentObject() const;
    virtual Element* anchorElement() const;
    virtual Element* actionElement() const;
    virtual KURL url() const;
    virtual bool isLink() const { return true; } 
    virtual String title() const;
    virtual String accessibilityDescription() const;
    
    virtual IntSize size() const;
    virtual IntRect elementRect() const;

private:    
    HTMLAreaElement* m_areaElement;
    HTMLMapElement* m_mapElement;
    AccessibilityObject* m_parent;
};
    
} // namespace WebCore

#endif // AccessibilityImageMapLink_h
