

#ifndef ResourceHandle_h
#define ResourceHandle_h

#include "AuthenticationChallenge.h"
#include "HTTPHeaderMap.h"
#include "ThreadableLoader.h"
#include <wtf/OwnPtr.h>

#if USE(SOUP)
typedef struct _SoupSession SoupSession;
#endif

#if PLATFORM(CF)
typedef const struct __CFData * CFDataRef;
#endif

#if PLATFORM(WIN)
typedef unsigned long DWORD;
typedef unsigned long DWORD_PTR;
typedef void* LPVOID;
typedef LPVOID HINTERNET;
typedef unsigned WPARAM;
typedef long LPARAM;
typedef struct HWND__* HWND;
typedef _W64 long LONG_PTR;
typedef LONG_PTR LRESULT;
#endif


#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#ifdef __OBJC__
@class NSData;
@class NSError;
@class NSURLConnection;
@class WebCoreResourceHandleAsDelegate;
#else
class NSData;
class NSError;
class NSURLConnection;
class WebCoreResourceHandleAsDelegate;
typedef struct objc_object *id;
#endif
#endif

#if USE(CFNETWORK)
typedef struct _CFURLConnection* CFURLConnectionRef;
typedef int CFHTTPCookieStorageAcceptPolicy;
typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
#endif

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class FormData;
class Frame;
class KURL;
class ResourceError;
class ResourceHandleClient;
class ResourceHandleInternal;
struct ResourceRequest;
class ResourceResponse;
class SchedulePair;
class SharedBuffer;

template <typename T> class Timer;

class ResourceHandle : public RefCounted<ResourceHandle> {
private:
    ResourceHandle(const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff, bool mightDownloadFromHandle);

    enum FailureType {
        BlockedFailure,
        InvalidURLFailure
    };

public:
    // FIXME: should not need the Frame
    static PassRefPtr<ResourceHandle> create(const ResourceRequest&, ResourceHandleClient*, Frame*, bool defersLoading, bool shouldContentSniff, bool mightDownloadFromHandle = false);

    static void loadResourceSynchronously(const ResourceRequest&, StoredCredentials, ResourceError&, ResourceResponse&, Vector<char>& data, Frame* frame);
    static bool willLoadFromCache(ResourceRequest&, Frame*);
#if PLATFORM(MAC)
    static bool didSendBodyDataDelegateExists();
#endif

    ~ResourceHandle();

#if PLATFORM(MAC) || USE(CFNETWORK)
    void willSendRequest(ResourceRequest&, const ResourceResponse& redirectResponse);
    bool shouldUseCredentialStorage();
#endif
#if PLATFORM(MAC) || USE(CFNETWORK) || USE(CURL)
    void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
    void receivedCredential(const AuthenticationChallenge&, const Credential&);
    void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);
    void receivedCancellation(const AuthenticationChallenge&);
#endif

#if PLATFORM(MAC)
    void didCancelAuthenticationChallenge(const AuthenticationChallenge&);
    NSURLConnection *connection() const;
    WebCoreResourceHandleAsDelegate *delegate();
    void releaseDelegate();
    id releaseProxy();

    void schedule(SchedulePair*);
    void unschedule(SchedulePair*);
#elif USE(CFNETWORK)
    static CFRunLoopRef loaderRunLoop();
    CFURLConnectionRef connection() const;
    CFURLConnectionRef releaseConnectionForDownload();
    static void setHostAllowsAnyHTTPSCertificate(const String&);
    static void setClientCertificate(const String& host, CFDataRef);
#endif

#if PLATFORM(WIN) && USE(CURL)
    static void setHostAllowsAnyHTTPSCertificate(const String&);
#endif
#if PLATFORM(WIN) && USE(CURL) && PLATFORM(CF)
    static void setClientCertificate(const String& host, CFDataRef);
#endif

    PassRefPtr<SharedBuffer> bufferedData();
    static bool supportsBufferedData();

    bool shouldContentSniff() const;
    static bool shouldContentSniffURL(const KURL&);

    static void forceContentSniffing();

#if USE(WININET)
    void setHasReceivedResponse(bool = true);
    bool hasReceivedResponse() const;
    void fileLoadTimer(Timer<ResourceHandle>*);
    void onHandleCreated(LPARAM);
    void onRequestRedirected(LPARAM);
    void onRequestComplete(LPARAM);
    friend void __stdcall transferJobStatusCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);
    friend LRESULT __stdcall ResourceHandleWndProc(HWND, unsigned message, WPARAM, LPARAM);
#endif

#if PLATFORM(QT) || USE(CURL) || USE(SOUP) || defined(ANDROID)
    ResourceHandleInternal* getInternal() { return d.get(); }
#endif

#if USE(SOUP)
    static SoupSession* defaultSession();
#endif

    // Used to work around the fact that you don't get any more NSURLConnection callbacks until you return from the one you're in.
    static bool loadsBlocked();    
    
    void clearAuthentication();
    void cancel();

    // The client may be 0, in which case no callbacks will be made.
    ResourceHandleClient* client() const;
    void setClient(ResourceHandleClient*);

    void setDefersLoading(bool);
      
    const ResourceRequest& request() const;

    void fireFailure(Timer<ResourceHandle>*);

private:
    void scheduleFailure(FailureType);

    bool start(Frame*);

    friend class ResourceHandleInternal;
    OwnPtr<ResourceHandleInternal> d;
};

}

#endif // ResourceHandle_h
