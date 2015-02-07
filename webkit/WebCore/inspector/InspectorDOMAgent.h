

#ifndef InspectorDOMAgent_h
#define InspectorDOMAgent_h

#include "EventListener.h"
#include "ScriptArray.h"
#include "ScriptObject.h"
#include "ScriptState.h"

#include <wtf/ListHashSet.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class Element;
    class Event;
    class Document;
    class InspectorFrontend;
    class NameNodeMap;
    class Node;
    class Page;

    class InspectorDOMAgent : public EventListener {
    public:
        InspectorDOMAgent(InspectorFrontend* frontend);
        ~InspectorDOMAgent();

        // Methods called from the frontend.
        void getChildNodes(long callId, long elementId);
        void setAttribute(long callId, long elementId, const String& name, const String& value);
        void removeAttribute(long callId, long elementId, const String& name);
        void setTextNodeValue(long callId, long elementId, const String& value);

        // Methods called from the InspectorController.
        void setDocument(Document* document);

        Node* nodeForId(long nodeId);
        long idForNode(Node* node);
        long pushNodePathToFrontend(Node* node);

   private:
        void startListening(Document* document);
        void stopListening(Document* document);

        virtual void handleEvent(Event* event, bool isWindowEvent);

        long bind(Node* node);
        void unbind(Node* node);

        void pushDocumentElementToFrontend();
        void pushChildNodesToFrontend(long elementId);

        ScriptObject buildObjectForNode(Node* node, int depth);
        ScriptArray buildArrayForElementAttributes(Element* elemen);
        ScriptArray buildArrayForElementChildren(Element* element, int depth);

        // We represent embedded doms as a part of the same hierarchy. Hence we treat children of frame owners differently.
        // We also skip whitespace text nodes conditionally. Following methods encapsulate these specifics.
        Node* innerFirstChild(Node* node);
        Node* innerNextSibling(Node* node);
        Node* innerPreviousSibling(Node* node);
        int innerChildNodeCount(Node* node);
        Element* innerParentElement(Node* node);
        bool isWhitespace(Node* node);

        Document* mainFrameDocument();
        void discardBindings();

        InspectorFrontend* m_frontend;
        HashMap<Node*, long> m_nodeToId;
        HashMap<long, Node*> m_idToNode;
        HashSet<long> m_childrenRequested;
        long m_lastNodeId;
        ListHashSet<RefPtr<Document> > m_documents;
        RefPtr<EventListener> m_eventListener;
    };


} // namespace WebCore

#endif // !defined(InspectorDOMAgent_h)
