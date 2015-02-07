
#ifndef Credential_h
#define Credential_h

#include "PlatformString.h"

namespace WebCore {

enum CredentialPersistence {
    CredentialPersistenceNone,
    CredentialPersistenceForSession,
    CredentialPersistencePermanent
};
    
class Credential {

public:
    Credential();
    Credential(const String& user, const String& password, CredentialPersistence);
    
    const String& user() const;
    const String& password() const;
    bool hasPassword() const;
    CredentialPersistence persistence() const;
    
private:
    String m_user;
    String m_password;
    CredentialPersistence m_persistence;
};

bool operator==(const Credential& a, const Credential& b);
inline bool operator!=(const Credential& a, const Credential& b) { return !(a == b); }
    
};
#endif
