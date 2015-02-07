

#import <Cocoa/Cocoa.h>

@class WebView;

@interface DrawTestToolbarController : NSObject {
    WebView *_drawView;
    NSMutableDictionary *_toolbarItems;
}

- (id)initWithDrawView:(WebView *)drawView;

@end
