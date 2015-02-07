

#ifndef DumpRenderTreeWin_h
#define DumpRenderTreeWin_h

struct IWebFrame;
struct IWebView;
struct PolicyDelegate;
typedef const struct __CFString* CFStringRef;
typedef struct HWND__* HWND;

extern IWebFrame* topLoadingFrame;
extern IWebFrame* frame;
extern PolicyDelegate* policyDelegate;

extern HWND webViewWindow;

#include <WebCore/COMPtr.h>
#include <string>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

std::wstring urlSuitableForTestResult(const std::wstring& url);
IWebView* createWebViewAndOffscreenWindow(HWND* webViewWindow = 0);
Vector<HWND>& openWindows();
typedef HashMap<HWND, COMPtr<IWebView> > WindowToWebViewMap;
WindowToWebViewMap& windowToWebViewMap();

void setPersistentUserStyleSheetLocation(CFStringRef);

extern UINT_PTR waitToDumpWatchdog;

#endif // DumpRenderTreeWin_h
