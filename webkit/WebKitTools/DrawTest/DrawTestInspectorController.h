

#import <Cocoa/Cocoa.h>


@interface DrawTestInspectorController : NSObject {
    IBOutlet NSPanel *_inspectorPanel;
}

+ (id)sharedInstance;

- (IBAction)showInspectorPanel:(id)sender;

@end
