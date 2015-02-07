

#include <WebCore/ChromeClient.h>
#include <WebCore/COMPtr.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/FocusDirection.h>
#include <WebCore/ScrollTypes.h>
#include <wtf/Forward.h>

class WebView;

interface IWebUIDelegate;

class WebChromeClient : public WebCore::ChromeClient {
public:
    WebChromeClient(WebView*);

    virtual void chromeDestroyed();

    virtual void setWindowRect(const WebCore::FloatRect&);
    virtual WebCore::FloatRect windowRect();
    
    virtual WebCore::FloatRect pageRect();
    
    virtual float scaleFactor();

    virtual void focus();
    virtual void unfocus();

    virtual bool canTakeFocus(WebCore::FocusDirection);
    virtual void takeFocus(WebCore::FocusDirection);

    virtual WebCore::Page* createWindow(WebCore::Frame*, const WebCore::FrameLoadRequest&, const WebCore::WindowFeatures&);
    virtual void show();

    virtual bool canRunModal();
    virtual void runModal();

    virtual void setToolbarsVisible(bool);
    virtual bool toolbarsVisible();
    
    virtual void setStatusbarVisible(bool);
    virtual bool statusbarVisible();
    
    virtual void setScrollbarsVisible(bool);
    virtual bool scrollbarsVisible();
    
    virtual void setMenubarVisible(bool);
    virtual bool menubarVisible();

    virtual void setResizable(bool);

    virtual void addMessageToConsole(WebCore::MessageSource source, WebCore::MessageType type, WebCore::MessageLevel level, const WebCore::String& message, unsigned line, const WebCore::String& url);

    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const WebCore::String& message, WebCore::Frame* frame);

    virtual void closeWindowSoon();

    virtual void runJavaScriptAlert(WebCore::Frame*, const WebCore::String&);
    virtual bool runJavaScriptConfirm(WebCore::Frame*, const WebCore::String&);
    virtual bool runJavaScriptPrompt(WebCore::Frame*, const WebCore::String& message, const WebCore::String& defaultValue, WebCore::String& result);
    virtual void setStatusbarText(const WebCore::String&);
    virtual bool shouldInterruptJavaScript();

    virtual bool tabsToLinks() const;
    virtual WebCore::IntRect windowResizerRect() const;

    virtual void repaint(const WebCore::IntRect&, bool contentChanged, bool immediate = false, bool repaintContentOnly = false);
    virtual void scroll(const WebCore::IntSize& scrollDelta, const WebCore::IntRect& rectToScroll, const WebCore::IntRect& clipRect);
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint& p) const;
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect& r) const;
    virtual PlatformWidget platformWindow() const;
    virtual void contentsSizeChanged(WebCore::Frame*, const WebCore::IntSize&) const;

    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags);

    virtual void setToolTip(const WebCore::String&, WebCore::TextDirection);

    virtual void print(WebCore::Frame*);

#if ENABLE(DATABASE)
    virtual void exceededDatabaseQuota(WebCore::Frame*, const WebCore::String&);
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded);
#endif

    virtual void populateVisitedLinks();

    virtual bool paintCustomScrollbar(WebCore::GraphicsContext*, const WebCore::FloatRect&, WebCore::ScrollbarControlSize, 
                                        WebCore::ScrollbarControlState, WebCore::ScrollbarPart pressedPart, bool vertical,
                                        float value, float proportion, WebCore::ScrollbarControlPartMask);
    virtual bool paintCustomScrollCorner(WebCore::GraphicsContext*, const WebCore::FloatRect&);

    virtual void runOpenPanel(WebCore::Frame*, PassRefPtr<WebCore::FileChooser>);

    virtual bool setCursor(WebCore::PlatformCursorHandle cursor);

    WebView* webView() const { return m_webView; }

    virtual void formStateDidChange(const WebCore::Node*) { }

    virtual PassOwnPtr<WebCore::HTMLParserQuirks> createHTMLParserQuirks() { return 0; }

    virtual void scrollRectIntoView(const WebCore::IntRect&, const WebCore::ScrollView*) const {}

    virtual void requestGeolocationPermissionForFrame(WebCore::Frame*, WebCore::Geolocation*);

private:
    COMPtr<IWebUIDelegate> uiDelegate();

    WebView* m_webView;
};
