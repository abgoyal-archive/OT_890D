
#ifndef AuthenticationMac_h
#define AuthenticationMac_h

#ifdef __OBJC__

@class NSURLAuthenticationChallenge;
@class NSURLCredential;
@class NSURLProtectionSpace;

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class ProtectionSpace;

NSURLAuthenticationChallenge *mac(const AuthenticationChallenge&);
NSURLProtectionSpace *mac(const ProtectionSpace&);
NSURLCredential *mac(const Credential&);

AuthenticationChallenge core(NSURLAuthenticationChallenge *);
ProtectionSpace core(NSURLProtectionSpace *);
Credential core(NSURLCredential *);

class WebCoreCredentialStorage {
public:
    static void set(NSURLCredential *credential, NSURLProtectionSpace *protectionSpace)
    {
        if (!m_storage)
            m_storage = [[NSMutableDictionary alloc] init];
        [m_storage setObject:credential forKey:protectionSpace];
    }

    static NSURLCredential *get(NSURLProtectionSpace *protectionSpace)
    {
        return static_cast<NSURLCredential *>([m_storage objectForKey:protectionSpace]);
    }

private:
    static NSMutableDictionary* m_storage;
};

}
#endif

#endif
