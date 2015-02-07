
#ifndef AuthenticationChallenge_h
#define AuthenticationChallenge_h

#include "AuthenticationChallengeBase.h"
#include "ResourceHandle.h"
#include <wtf/RefPtr.h>

typedef struct _CFURLAuthChallenge* CFURLAuthChallengeRef;

namespace WebCore {

class ResourceHandle;

class AuthenticationChallenge : public AuthenticationChallengeBase {
public:
    AuthenticationChallenge() {}
    AuthenticationChallenge(const ProtectionSpace& protectionSpace, const Credential& proposedCredential, unsigned previousFailureCount, const ResourceResponse& response, const ResourceError& error);
    AuthenticationChallenge(CFURLAuthChallengeRef, ResourceHandle* sourceHandle);

    ResourceHandle* sourceHandle() const { return m_sourceHandle.get(); }
    CFURLAuthChallengeRef cfURLAuthChallengeRef() const { return m_cfChallenge.get(); }

private:
    friend class AuthenticationChallengeBase;
    static bool platformCompare(const AuthenticationChallenge& a, const AuthenticationChallenge& b);

    RefPtr<ResourceHandle> m_sourceHandle;
    RetainPtr<CFURLAuthChallengeRef> m_cfChallenge;
};

}

#endif
