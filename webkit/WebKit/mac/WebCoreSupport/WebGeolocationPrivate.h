

#import <Foundation/Foundation.h>

@class WebGeolocationPrivate;

@interface WebGeolocation : NSObject {
@private
    WebGeolocationPrivate *_private;
}

- (BOOL)shouldClearCache;
- (void)setIsAllowed:(BOOL)allowed;
@end
