

#ifndef AuthenticationChallenge_h
#define AuthenticationChallenge_h

#include "AuthenticationChallengeBase.h"
#include "ResourceHandle.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class ResourceHandle;

    class AuthenticationChallenge : public AuthenticationChallengeBase {
    public:
        AuthenticationChallenge() {}
        AuthenticationChallenge(const ProtectionSpace&, const Credential& proposedCredential, unsigned previousFailureCount, const ResourceResponse&, const ResourceError&);

        ResourceHandle* sourceHandle() const { return m_sourceHandle.get(); }

    private:
        friend class AuthenticationChallengeBase;
        static bool platformCompare(const AuthenticationChallenge&, const AuthenticationChallenge&);

        RefPtr<ResourceHandle> m_sourceHandle;
    };

} // namespace WebCore

#endif
