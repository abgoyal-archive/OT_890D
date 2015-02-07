

#ifndef CachedPage_h
#define CachedPage_h

#include "CachedFrame.h"

namespace WebCore {
    
    class CachedFramePlatformData;
    class DOMWindow;
    class Document;
    class DocumentLoader;
    class FrameView;
    class KURL;
    class Node;
    class Page;

class CachedPage : public RefCounted<CachedPage> {
public:
    static PassRefPtr<CachedPage> create(Page*);
    ~CachedPage();

    void restore(Page*);
    void clear();

    Document* document() const { return m_cachedMainFrame->document(); }
    DocumentLoader* documentLoader() const { return m_cachedMainFrame->documentLoader(); }
    FrameView* mainFrameView() const { return m_cachedMainFrame->view(); }
    const KURL& url() const { return m_cachedMainFrame->url(); }
    DOMWindow* domWindow() const { return m_cachedMainFrame->domWindow(); }

    double timeStamp() const { return m_timeStamp; }
    
    CachedFrame* cachedMainFrame() { return m_cachedMainFrame.get(); }

private:
    CachedPage(Page*);

    double m_timeStamp;
    RefPtr<CachedFrame> m_cachedMainFrame;
};

} // namespace WebCore

#endif // CachedPage_h

