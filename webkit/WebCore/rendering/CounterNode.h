

#ifndef CounterNode_h
#define CounterNode_h

#include <wtf/Noncopyable.h>

// This implements a counter tree that is used for finding parents in counters() lookup,
// and for propagating count changes when nodes are added or removed.

// Parents represent unique counters and their scope, which are created either explicitly
// by "counter-reset" style rules or implicitly by referring to a counter that is not in scope.
// Such nodes are tagged as "reset" nodes, although they are not all due to "counter-reset".

// Not that render tree children are often counter tree siblings due to counter scoping rules.

namespace WebCore {

class RenderObject;

class CounterNode : public Noncopyable {
public:
    CounterNode(RenderObject*, bool isReset, int value);

    bool isReset() const { return m_isReset; }
    int value() const { return m_value; }
    int countInParent() const { return m_countInParent; }
    RenderObject* renderer() const { return m_renderer; }

    CounterNode* parent() const { return m_parent; }
    CounterNode* previousSibling() const { return m_previousSibling; }
    CounterNode* nextSibling() const { return m_nextSibling; }
    CounterNode* firstChild() const { return m_firstChild; }
    CounterNode* lastChild() const { return m_lastChild; }

    void insertAfter(CounterNode* newChild, CounterNode* beforeChild);
    void removeChild(CounterNode*);

private:
    int computeCountInParent() const;
    void recount();

    bool m_isReset;
    int m_value;
    int m_countInParent;
    RenderObject* m_renderer;

    CounterNode* m_parent;
    CounterNode* m_previousSibling;
    CounterNode* m_nextSibling;
    CounterNode* m_firstChild;
    CounterNode* m_lastChild;
};

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::CounterNode*);
#endif

#endif // CounterNode_h
