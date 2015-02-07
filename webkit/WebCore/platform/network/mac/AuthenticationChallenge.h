
#ifndef AuthenticationChallenge_h
#define AuthenticationChallenge_h

#include "AuthenticationChallengeBase.h"

#include <wtf/RetainPtr.h>
#ifndef __OBJC__
typedef struct objc_object *id;
class NSURLAuthenticationChallenge;
#else
@class NSURLAuthenticationChallenge;
#endif

namespace WebCore {

class AuthenticationChallenge : public AuthenticationChallengeBase {
public:
    AuthenticationChallenge() {}
    AuthenticationChallenge(const ProtectionSpace& protectionSpace, const Credential& proposedCredential, unsigned previousFailureCount, const ResourceResponse& response, const ResourceError& error);
    AuthenticationChallenge(NSURLAuthenticationChallenge *);

    id sender() const { return m_sender.get(); }
    NSURLAuthenticationChallenge *nsURLAuthenticationChallenge() const { return m_macChallenge.get(); }

private:
    friend class AuthenticationChallengeBase;
    static bool platformCompare(const AuthenticationChallenge& a, const AuthenticationChallenge& b);

    RetainPtr<id> m_sender;
    RetainPtr<NSURLAuthenticationChallenge *> m_macChallenge;
};

}

#endif
