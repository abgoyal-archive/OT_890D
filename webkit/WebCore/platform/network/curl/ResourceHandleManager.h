

#ifndef ResourceHandleManager_h
#define ResourceHandleManager_h

#include "Frame.h"
#include "Timer.h"
#include "ResourceHandleClient.h"

#if PLATFORM(WIN)
#include <winsock2.h>
#include <windows.h>
#endif

#include <curl/curl.h>
#include <wtf/Vector.h>

namespace WebCore {

class ResourceHandleManager {
public:
    static ResourceHandleManager* sharedInstance();
    void add(ResourceHandle*);
    void cancel(ResourceHandle*);
    void setCookieJarFileName(const char* cookieJarFileName);

    void dispatchSynchronousJob(ResourceHandle*);

    void setupPOST(ResourceHandle*, struct curl_slist**);
    void setupPUT(ResourceHandle*, struct curl_slist**);

private:
    ResourceHandleManager();
    ~ResourceHandleManager();
    void downloadTimerCallback(Timer<ResourceHandleManager>*);
    void removeFromCurl(ResourceHandle*);
    bool removeScheduledJob(ResourceHandle*);
    void startJob(ResourceHandle*);
    bool startScheduledJobs();

    void initializeHandle(ResourceHandle*);

    Timer<ResourceHandleManager> m_downloadTimer;
    CURLM* m_curlMultiHandle;
    CURLSH* m_curlShareHandle;
    char* m_cookieJarFileName;
    char m_curlErrorBuffer[CURL_ERROR_SIZE];
    Vector<ResourceHandle*> m_resourceHandleList;
    int m_runningJobs;
};

}

#endif
