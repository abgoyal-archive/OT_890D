
 
#ifndef CachedFrame_h
#define CachedFrame_h

#include "KURL.h"
#include "ScriptCachedFrameData.h"
#include <wtf/RefPtr.h>

namespace WebCore {
    
    class CachedFrame;
    class CachedFramePlatformData;
    class DOMWindow;
    class Document;
    class DocumentLoader;
    class Frame;
    class FrameView;
    class Node;

typedef Vector<RefPtr<CachedFrame> > CachedFrameVector;

class CachedFrame : public RefCounted<CachedFrame> {
public:
    static PassRefPtr<CachedFrame> create(Frame* frame) { return adoptRef(new CachedFrame(frame)); }
    ~CachedFrame();

    void restore();
    void clear();

    Document* document() const { return m_document.get(); }
    DocumentLoader* documentLoader() const { return m_documentLoader.get(); }
    FrameView* view() const { return m_view.get(); }
    Node* mousePressNode() const { return m_mousePressNode.get(); }
    const KURL& url() const { return m_url; }
    DOMWindow* domWindow() const { return m_cachedFrameScriptData->domWindow(); }

    void setCachedFramePlatformData(CachedFramePlatformData*);
    CachedFramePlatformData* cachedFramePlatformData();
    
    int descendantFrameCount() const;

private:
    CachedFrame(Frame*);

    RefPtr<Document> m_document;
    RefPtr<DocumentLoader> m_documentLoader;
    RefPtr<FrameView> m_view;
    RefPtr<Node> m_mousePressNode;
    KURL m_url;
    OwnPtr<ScriptCachedFrameData> m_cachedFrameScriptData;
    OwnPtr<CachedFramePlatformData> m_cachedFramePlatformData;
    
    CachedFrameVector m_childFrames;
};

} // namespace WebCore

#endif // CachedFrame_h
