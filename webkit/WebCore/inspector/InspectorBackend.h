

#ifndef InspectorBackend_h
#define InspectorBackend_h

#include "Console.h"
#include "InspectorController.h"
#include "PlatformString.h"

#include <wtf/RefCounted.h>

namespace WebCore {

class CachedResource;
class InspectorClient;
class InspectorDOMAgent;
class JavaScriptCallFrame;
class Node;

class InspectorBackend : public RefCounted<InspectorBackend>
{
public:
    static PassRefPtr<InspectorBackend> create(InspectorController* inspectorController, InspectorClient* client)
    {
        return adoptRef(new InspectorBackend(inspectorController, client));
    }

    ~InspectorBackend();

    InspectorController* inspectorController() { return m_inspectorController; }
    
    void disconnectController() { m_inspectorController = 0; }

    void hideDOMNodeHighlight();

    String localizedStringsURL();
    String hiddenPanels();

    void windowUnloading();

    bool isWindowVisible();

    void addResourceSourceToFrame(long identifier, Node* frame);
    bool addSourceToFrame(const String& mimeType, const String& source, Node* frame);

    void clearMessages();

    void toggleNodeSearch();

    void attach();
    void detach();

    void setAttachedWindowHeight(unsigned height);

    void storeLastActivePanel(const String& panelName);

    bool searchingForNode();

    void loaded(bool enableDOMAgent);

    void enableResourceTracking(bool always);
    void disableResourceTracking(bool always);
    bool resourceTrackingEnabled() const;

    void moveWindowBy(float x, float y) const;
    void closeWindow();

    const String& platform() const;

#if ENABLE(JAVASCRIPT_DEBUGGER)
    const ProfilesArray& profiles() const;

    void startProfiling();
    void stopProfiling();

    void enableProfiler(bool always);
    void disableProfiler(bool always);
    bool profilerEnabled();

    void enableDebugger(bool always);
    void disableDebugger(bool always);
    bool debuggerEnabled() const;

    JavaScriptCallFrame* currentCallFrame() const;

    void addBreakpoint(const String& sourceID, unsigned lineNumber);
    void removeBreakpoint(const String& sourceID, unsigned lineNumber);

    bool pauseOnExceptions();
    void setPauseOnExceptions(bool pause);

    void pauseInDebugger();
    void resumeDebugger();

    void stepOverStatementInDebugger();
    void stepIntoStatementInDebugger();
    void stepOutOfFunctionInDebugger();
#endif

    void getChildNodes(long callId, long elementId);
    void setAttribute(long callId, long elementId, const String& name, const String& value);
    void removeAttribute(long callId, long elementId, const String& name);
    void setTextNodeValue(long callId, long elementId, const String& value);

    // Generic code called from custom implementations.
    void highlight(Node* node);

private:
    InspectorBackend(InspectorController* inspectorController, InspectorClient* client);

    InspectorController* m_inspectorController;
    InspectorClient* m_client;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    ProfilesArray m_emptyProfiles;
#endif
};

} // namespace WebCore

#endif // !defined(InspectorBackend_h)
