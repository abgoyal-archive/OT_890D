

#ifndef AuthenticationCF_h
#define AuthenticationCF_h

typedef struct _CFURLAuthChallenge* CFURLAuthChallengeRef;
typedef struct _CFURLCredential* CFURLCredentialRef;
typedef struct _CFURLProtectionSpace* CFURLProtectionSpaceRef;

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class ProtectionSpace;

CFURLAuthChallengeRef createCF(const AuthenticationChallenge&);
CFURLCredentialRef createCF(const Credential&);
CFURLProtectionSpaceRef createCF(const ProtectionSpace&);

Credential core(CFURLCredentialRef);
ProtectionSpace core(CFURLProtectionSpaceRef);

class WebCoreCredentialStorage {
public:
    static void set(CFURLProtectionSpaceRef protectionSpace, CFURLCredentialRef credential)
    {
        if (!m_storage)
            m_storage = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(m_storage, protectionSpace, credential);
    }

    static CFURLCredentialRef get(CFURLProtectionSpaceRef protectionSpace)
    {
        if (!m_storage)
            return 0;
        return (CFURLCredentialRef)CFDictionaryGetValue(m_storage, protectionSpace);
    }

private:
    static CFMutableDictionaryRef m_storage;
};

}

#endif
