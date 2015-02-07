
#ifndef ProtectionSpace_h
#define ProtectionSpace_h

#include "PlatformString.h"

namespace WebCore {

enum ProtectionSpaceServerType {
  ProtectionSpaceServerHTTP = 1,
  ProtectionSpaceServerHTTPS = 2,
  ProtectionSpaceServerFTP = 3,
  ProtectionSpaceServerFTPS = 4,
  ProtectionSpaceProxyHTTP = 5,
  ProtectionSpaceProxyHTTPS = 6,
  ProtectionSpaceProxyFTP = 7,
  ProtectionSpaceProxySOCKS = 8
};

enum ProtectionSpaceAuthenticationScheme {
  ProtectionSpaceAuthenticationSchemeDefault = 1,
  ProtectionSpaceAuthenticationSchemeHTTPBasic = 2,
  ProtectionSpaceAuthenticationSchemeHTTPDigest = 3,
  ProtectionSpaceAuthenticationSchemeHTMLForm = 4,
  ProtectionSpaceAuthenticationSchemeNTLM = 5,
  ProtectionSpaceAuthenticationSchemeNegotiate = 6,
};

class ProtectionSpace {

public:
    ProtectionSpace();
    ProtectionSpace(const String& host, int port, ProtectionSpaceServerType, const String& realm, ProtectionSpaceAuthenticationScheme);
    
    const String& host() const;
    int port() const;
    ProtectionSpaceServerType serverType() const;
    bool isProxy() const;
    const String& realm() const;
    ProtectionSpaceAuthenticationScheme authenticationScheme() const;
    
    bool receivesCredentialSecurely() const;

private:
    String m_host;
    int m_port;
    ProtectionSpaceServerType m_serverType;
    String m_realm;
    ProtectionSpaceAuthenticationScheme m_authenticationScheme;
};

bool operator==(const ProtectionSpace& a, const ProtectionSpace& b);
inline bool operator!=(const ProtectionSpace& a, const ProtectionSpace& b) { return !(a == b); }
    
}
#endif
