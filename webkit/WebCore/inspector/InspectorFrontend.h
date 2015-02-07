

#ifndef InspectorFrontend_h
#define InspectorFrontend_h

#include "ScriptArray.h"
#include "ScriptObject.h"
#include "ScriptState.h"
#include <wtf/PassOwnPtr.h>

#if ENABLE(JAVASCRIPT_DEBUGGER)
namespace JSC {
    class JSValue;
    class SourceCode;
    class UString;
}
#endif

namespace WebCore {
    class ConsoleMessage;
    class InspectorResource;
    class Node;
    class ScriptFunctionCall;
    class ScriptString;

    class InspectorFrontend {
    public:
        InspectorFrontend(ScriptState*, ScriptObject webInspector);
        ~InspectorFrontend();

        ScriptArray newScriptArray();
        ScriptObject newScriptObject();

        void addMessageToConsole(const ScriptObject& messageObj, const Vector<ScriptString>& frames, const Vector<ScriptValue> wrappedArguments, const String& message);
        
        bool addResource(long long identifier, const ScriptObject& resourceObj);
        bool updateResource(long long identifier, const ScriptObject& resourceObj);
        void removeResource(long long identifier);

        void updateFocusedNode(Node* node);
        void setAttachedWindow(bool attached);
        void inspectedWindowScriptObjectCleared(Frame* frame);
        void showPanel(int panel);
        void populateInterface();
        void reset();

        void resourceTrackingWasEnabled();
        void resourceTrackingWasDisabled();

#if ENABLE(JAVASCRIPT_DEBUGGER)
        void attachDebuggerWhenShown();
        void debuggerWasEnabled();
        void debuggerWasDisabled();
        void profilerWasEnabled();
        void profilerWasDisabled();
        void parsedScriptSource(const JSC::SourceCode&);
        void failedToParseScriptSource(const JSC::SourceCode&, int errorLine, const JSC::UString& errorMessage);
        void addProfile(const JSC::JSValue& profile);
        void setRecordingProfile(bool isProfiling);
        void pausedScript();
        void resumedScript();
#endif

#if ENABLE(DATABASE)
        bool addDatabase(const ScriptObject& dbObj);
#endif
        
#if ENABLE(DOM_STORAGE)
        bool addDOMStorage(const ScriptObject& domStorageObj);
#endif

        void setDocumentElement(const ScriptObject& root);
        void setChildNodes(int parentId, const ScriptArray& nodes);
        void hasChildrenUpdated(int id, bool newValue);
        void childNodeInserted(int parentId, int prevId, const ScriptObject& node);
        void childNodeRemoved(int parentId, int id);
        void attributesUpdated(int id, const ScriptArray& attributes);
        void didGetChildNodes(int callId);
        void didApplyDomChange(int callId, bool success);

    private:
        PassOwnPtr<ScriptFunctionCall> newFunctionCall(const String& functionName);
        void callSimpleFunction(const String& functionName);
        ScriptState* m_scriptState;
        ScriptObject m_webInspector;
    };

} // namespace WebCore

#endif // !defined(InspectorFrontend_h)
