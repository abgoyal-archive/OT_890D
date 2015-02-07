

#ifndef NodeIterator_h
#define NodeIterator_h

#include "NodeFilter.h"
#include "Traversal.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    typedef int ExceptionCode;

    class NodeIterator : public RefCounted<NodeIterator>, public Traversal {
    public:
        static PassRefPtr<NodeIterator> create(PassRefPtr<Node> rootNode, unsigned whatToShow, PassRefPtr<NodeFilter> filter, bool expandEntityReferences)
        {
            return adoptRef(new NodeIterator(rootNode, whatToShow, filter, expandEntityReferences));
        }
        ~NodeIterator();

        PassRefPtr<Node> nextNode(ScriptState*, ExceptionCode&);
        PassRefPtr<Node> previousNode(ScriptState*, ExceptionCode&);
        void detach();

        Node* referenceNode() const { return m_referenceNode.node.get(); }
        bool pointerBeforeReferenceNode() const { return m_referenceNode.isPointerBeforeNode; }

        // This function is called before any node is removed from the document tree.
        void nodeWillBeRemoved(Node*);

        // For non-JS bindings. Silently ignores the JavaScript exception if any.
        PassRefPtr<Node> nextNode(ExceptionCode& ec) { return nextNode(scriptStateFromNode(referenceNode()), ec); }
        PassRefPtr<Node> previousNode(ExceptionCode& ec) { return previousNode(scriptStateFromNode(referenceNode()), ec); }

    private:
        NodeIterator(PassRefPtr<Node>, unsigned whatToShow, PassRefPtr<NodeFilter>, bool expandEntityReferences);

        struct NodePointer {
            RefPtr<Node> node;
            bool isPointerBeforeNode;
            NodePointer();
            NodePointer(PassRefPtr<Node>, bool);
            void clear();
            bool moveToNext(Node* root);
            bool moveToPrevious(Node* root);
        };

        void updateForNodeRemoval(Node* nodeToBeRemoved, NodePointer&) const;

        NodePointer m_referenceNode;
        NodePointer m_candidateNode;
        bool m_detached;
    };

} // namespace WebCore

#endif // NodeIterator_h
