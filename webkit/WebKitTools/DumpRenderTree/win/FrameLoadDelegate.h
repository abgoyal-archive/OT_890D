

#ifndef FrameLoadDelegate_h
#define FrameLoadDelegate_h

#include <WebKit/WebKit.h>
#include <wtf/OwnPtr.h>

class AccessibilityController;
class GCController;

class FrameLoadDelegate : public IWebFrameLoadDelegate, public IWebFrameLoadDelegatePrivate {
public:
    FrameLoadDelegate();
    virtual ~FrameLoadDelegate();

    void processWork();

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebFrameLoadDelegate
    virtual HRESULT STDMETHODCALLTYPE didStartProvisionalLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame); 

    virtual HRESULT STDMETHODCALLTYPE didReceiveServerRedirectForProvisionalLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didFailProvisionalLoadWithError( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebError *error,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didCommitLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didReceiveTitle( 
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR title,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didReceiveIcon( 
        /* [in] */ IWebView *webView,
        /* [in] */ OLE_HANDLE image,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 

    virtual HRESULT STDMETHODCALLTYPE didFinishLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didFailLoadWithError( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebError *error,
        /* [in] */ IWebFrame *forFrame);

    virtual HRESULT STDMETHODCALLTYPE didChangeLocationWithinPageForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 

    virtual HRESULT STDMETHODCALLTYPE willPerformClientRedirectToURL( 
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR url,
        /* [in] */ double delaySeconds,
        /* [in] */ DATE fireDate,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didCancelClientRedirectForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE willCloseFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE windowScriptObjectAvailable( 
        /* [in] */ IWebView *sender,
        /* [in] */ JSContextRef context,
        /* [in] */ JSObjectRef windowObject) { return E_NOTIMPL; }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE didClearWindowObject( 
        /* [in] */ IWebView* webView,
        /* [in] */ JSContextRef context,
        /* [in] */ JSObjectRef windowObject,
        /* [in] */ IWebFrame* frame);

    // IWebFrameLoadDelegatePrivate
    virtual HRESULT STDMETHODCALLTYPE didFinishDocumentLoadForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame);
        
    virtual HRESULT STDMETHODCALLTYPE didFirstLayoutInFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 
        
    virtual HRESULT STDMETHODCALLTYPE didHandleOnloadEventsForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didFirstVisuallyNonEmptyLayoutInFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame);

protected:
    void locationChangeDone(IWebError*, IWebFrame*);

    ULONG m_refCount;
    OwnPtr<GCController> m_gcController;
    OwnPtr<AccessibilityController> m_accessibilityController;
};

#endif // FrameLoadDelegate_h
