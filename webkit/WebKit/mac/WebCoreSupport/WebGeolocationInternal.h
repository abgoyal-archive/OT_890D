

#import "WebGeolocationPrivate.h"

namespace WebCore {
    class Geolocation;
}

typedef WebCore::Geolocation WebCoreGeolocation;

@interface WebGeolocation (WebInternal)

- (id)_initWithWebCoreGeolocation:(WebCoreGeolocation *)geolocation;

@end
