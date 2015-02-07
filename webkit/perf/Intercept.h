

#ifndef INTERCEPT_H
#define INTERCEPT_H

#include "jni_utility.h"
#include "MyJavaVM.h"
#include "PlatformString.h"
#include "Timer.h"
#include "Vector.h"
#include "WebCoreFrameBridge.h"
#include "WebCoreResourceLoader.h"

namespace WebCore {
    class Page;
    class ResourceHandle;
    class ResourceRequest;
}

using namespace android;
using namespace WebCore;
using namespace WTF;

class MyResourceLoader : public WebCoreResourceLoader {
public:
    MyResourceLoader(ResourceHandle* handle, String url)
        : WebCoreResourceLoader(JSC::Bindings::getJNIEnv(), MY_JOBJECT)
        , m_handle(handle)
        , m_url(url) {}

    void handleRequest();

private:
    void loadData(const String&);
    void loadFile(const String&);
    ResourceHandle* m_handle;
    String m_url;
};

class MyWebFrame : public WebFrame {
public:
    MyWebFrame(Page* page)
        : WebFrame(JSC::Bindings::getJNIEnv(), MY_JOBJECT, MY_JOBJECT, page)
        , m_timer(this, &MyWebFrame::timerFired) {}

    virtual WebCoreResourceLoader* startLoadingResource(ResourceHandle* handle,
            const ResourceRequest& req, bool, bool);

    virtual bool canHandleRequest(const ResourceRequest&) { return true; }

private:
    void timerFired(Timer<MyWebFrame>*);
    Vector<MyResourceLoader*> m_requests;
    Timer<MyWebFrame> m_timer;
};

#endif
