

#ifndef SSLKeyGenerator_h
#define SSLKeyGenerator_h

#include <wtf/Vector.h>
#include "PlatformString.h"

namespace WebCore {

    class KURL;

    void getSupportedKeySizes(Vector<String>&);
    String signedPublicKeyAndChallengeString(unsigned keySizeIndex, const String& challengeString, const KURL&);

} // namespace WebCore

#endif // SSLKeyGenerator_h
