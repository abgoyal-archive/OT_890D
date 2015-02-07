

#import <Cocoa/Cocoa.h>


@interface AppDelegate : NSObject {
    IBOutlet NSWindow *svgImageRepTestWindow;
}

- (IBAction)showTestsPanel:(id)sender;
- (IBAction)showImageRepTestWindow:(id)sender;
- (IBAction)showInspectorPanel:(id)sender;

@end
