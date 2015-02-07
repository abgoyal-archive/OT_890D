

#ifndef AccessibilityUIElement_h
#define AccessibilityUIElement_h

#include <JavaScriptCore/JSObjectRef.h>
#include <wtf/Platform.h>
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#ifdef __OBJC__
typedef id PlatformUIElement;
#else
typedef struct objc_object* PlatformUIElement;
#endif
#elif PLATFORM(WIN)
#undef _WINSOCKAPI_
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h in windows.h

#include <oleacc.h>
#include <WebCore/COMPtr.h>

typedef COMPtr<IAccessible> PlatformUIElement;
#elif PLATFORM(GTK)
#include <atk/atk.h>
typedef AtkObject* PlatformUIElement;
#else
typedef void* PlatformUIElement;
#endif

class AccessibilityUIElement {
public:
    AccessibilityUIElement(PlatformUIElement);
    AccessibilityUIElement(const AccessibilityUIElement&);
    ~AccessibilityUIElement();

    PlatformUIElement platformUIElement() { return m_element; }

    static JSObjectRef makeJSAccessibilityUIElement(JSContextRef, const AccessibilityUIElement&);

    void getLinkedUIElements(Vector<AccessibilityUIElement>&);
    void getDocumentLinks(Vector<AccessibilityUIElement>&);
    void getChildren(Vector<AccessibilityUIElement>&);
    void getChildrenWithRange(Vector<AccessibilityUIElement>&, unsigned location, unsigned length);
    
    AccessibilityUIElement elementAtPoint(int x, int y);
    AccessibilityUIElement getChildAtIndex(unsigned);
    int childrenCount();
    AccessibilityUIElement titleUIElement();
    AccessibilityUIElement parentElement();
    
    // Methods - platform-independent implementations
    JSStringRef allAttributes();
    JSStringRef attributesOfLinkedUIElements();
    JSStringRef attributesOfDocumentLinks();
    JSStringRef attributesOfChildren();
    JSStringRef parameterizedAttributeNames();
    void increment();
    void decrement();

    // Attributes - platform-independent implementations
    JSStringRef attributeValue(JSStringRef attribute);
    bool isAttributeSettable(JSStringRef attribute);
    bool isActionSupported(JSStringRef action);
    JSStringRef role();
    JSStringRef title();
    JSStringRef description();
    JSStringRef language();
    double x();
    double y();
    double width();
    double height();
    double intValue();
    double minValue();
    double maxValue();
    JSStringRef valueDescription();
    int insertionPointLineNumber();
    JSStringRef selectedTextRange();
    bool isEnabled();
    bool isRequired() const;
    double clickPointX();
    double clickPointY();

    // Table-specific attributes
    JSStringRef attributesOfColumnHeaders();
    JSStringRef attributesOfRowHeaders();
    JSStringRef attributesOfColumns();
    JSStringRef attributesOfRows();
    JSStringRef attributesOfVisibleCells();
    JSStringRef attributesOfHeader();
    int indexInTable();
    JSStringRef rowIndexRange();
    JSStringRef columnIndexRange();
    
    // Parameterized attributes
    int lineForIndex(int);
    JSStringRef boundsForRange(unsigned location, unsigned length);
    void setSelectedTextRange(unsigned location, unsigned length);
    
    // Table-specific
    AccessibilityUIElement cellForColumnAndRow(unsigned column, unsigned row);

private:
    static JSClassRef getJSClass();

    PlatformUIElement m_element;
};

#endif // AccessibilityUIElement_h
