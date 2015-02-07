

#import <Cocoa/Cocoa.h>

#import <WebKit/WebView.h>

@interface DrawTestView : WebView
{

}

- (void)setDocument:(NSURL *)documentURL;

@end
