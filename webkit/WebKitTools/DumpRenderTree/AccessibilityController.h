

#ifndef AccessibilityController_h
#define AccessibilityController_h

#include <JavaScriptCore/JSObjectRef.h>

class AccessibilityUIElement;

class AccessibilityController {
public:
    AccessibilityController();
    ~AccessibilityController();

    void makeWindowObject(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception);

    // Controller Methods - platform-independent implementations
    AccessibilityUIElement rootElement();
    AccessibilityUIElement focusedElement();

private:
    static JSClassRef getJSClass();
};

#endif // AccessibilityController_h
