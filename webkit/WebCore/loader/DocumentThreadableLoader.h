

#ifndef DocumentThreadableLoader_h
#define DocumentThreadableLoader_h

#include "SubresourceLoaderClient.h"
#include "ThreadableLoader.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class Document;
    struct ResourceRequest;
    class ThreadableLoaderClient;

    class DocumentThreadableLoader : public RefCounted<DocumentThreadableLoader>, public ThreadableLoader, private SubresourceLoaderClient  {
    public:
        static void loadResourceSynchronously(Document*, const ResourceRequest&, ThreadableLoaderClient&, StoredCredentials);
        static PassRefPtr<DocumentThreadableLoader> create(Document*, ThreadableLoaderClient*, const ResourceRequest&, LoadCallbacks, ContentSniff, StoredCredentials, CrossOriginRedirectPolicy);
        virtual ~DocumentThreadableLoader();

        virtual void cancel();

        using RefCounted<DocumentThreadableLoader>::ref;
        using RefCounted<DocumentThreadableLoader>::deref;

    protected:
        virtual void refThreadableLoader() { ref(); }
        virtual void derefThreadableLoader() { deref(); }

    private:
        DocumentThreadableLoader(Document*, ThreadableLoaderClient*, const ResourceRequest&, LoadCallbacks, ContentSniff, StoredCredentials, CrossOriginRedirectPolicy);
        virtual void willSendRequest(SubresourceLoader*, ResourceRequest&, const ResourceResponse& redirectResponse);
        virtual void didSendData(SubresourceLoader*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent);

        virtual void didReceiveResponse(SubresourceLoader*, const ResourceResponse&);
        virtual void didReceiveData(SubresourceLoader*, const char*, int lengthReceived);
        virtual void didFinishLoading(SubresourceLoader*);
        virtual void didFail(SubresourceLoader*, const ResourceError&);

        virtual bool getShouldUseCredentialStorage(SubresourceLoader*, bool& shouldUseCredentialStorage);
        virtual void didReceiveAuthenticationChallenge(SubresourceLoader*, const AuthenticationChallenge&);
        virtual void receivedCancellation(SubresourceLoader*, const AuthenticationChallenge&);

        RefPtr<SubresourceLoader> m_loader;
        ThreadableLoaderClient* m_client;
        Document* m_document;
        bool m_allowStoredCredentials;
        bool m_sameOriginRequest;
        bool m_denyCrossOriginRedirect;
    };

} // namespace WebCore

#endif // DocumentThreadableLoader_h
