

#ifndef ThreadableLoader_h
#define ThreadableLoader_h

#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class ResourceError;
    struct ResourceRequest;
    class ResourceResponse;
    class ScriptExecutionContext;
    class ThreadableLoaderClient;

    enum LoadCallbacks {
        SendLoadCallbacks,
        DoNotSendLoadCallbacks
    };

    enum ContentSniff {
        SniffContent,
        DoNotSniffContent
    };

    enum StoredCredentials {
        AllowStoredCredentials,
        DoNotAllowStoredCredentials
    };

    enum CrossOriginRedirectPolicy {
        DenyCrossOriginRedirect,
        AllowCrossOriginRedirect
    };

    // Useful for doing loader operations from any thread (not threadsafe, 
    // just able to run on threads other than the main thread).
    class ThreadableLoader : public Noncopyable {
    public:
        static void loadResourceSynchronously(ScriptExecutionContext*, const ResourceRequest&, ThreadableLoaderClient&, StoredCredentials);
        static PassRefPtr<ThreadableLoader> create(ScriptExecutionContext*, ThreadableLoaderClient*, const ResourceRequest&, LoadCallbacks, ContentSniff, StoredCredentials, CrossOriginRedirectPolicy);

        virtual void cancel() = 0;
        void ref() { refThreadableLoader(); }
        void deref() { derefThreadableLoader(); }

    protected:
        virtual ~ThreadableLoader() { }
        virtual void refThreadableLoader() = 0;
        virtual void derefThreadableLoader() = 0;
    };

} // namespace WebCore

#endif // ThreadableLoader_h
