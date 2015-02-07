

#ifndef ScriptEventListener_h
#define ScriptEventListener_h

#include "JSLazyEventListener.h"

#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Attribute;
    class Frame;
    class Node;

    PassRefPtr<JSLazyEventListener> createAttributeEventListener(Node*, Attribute*);
    PassRefPtr<JSLazyEventListener> createAttributeEventListener(Frame*, Attribute*);

} // namespace WebCore

#endif // ScriptEventListener_h
