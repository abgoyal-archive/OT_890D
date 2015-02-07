
#ifndef AuthenticationChallenge_h
#define AuthenticationChallenge_h

#include "AuthenticationChallengeBase.h"
#include "ResourceHandle.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class ResourceHandle;

class AuthenticationChallenge : public AuthenticationChallengeBase {
public:
    AuthenticationChallenge()
    {
    }

    AuthenticationChallenge(const ProtectionSpace& protectionSpace, const Credential& proposedCredential, unsigned previousFailureCount, const ResourceResponse& response, const ResourceError& error)
        : AuthenticationChallengeBase(protectionSpace, proposedCredential, previousFailureCount, response, error)
    {
    }

    ResourceHandle* sourceHandle() const { return m_sourceHandle.get(); }

    RefPtr<ResourceHandle> m_sourceHandle;    
};

}

#endif
