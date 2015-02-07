

#import <Cocoa/Cocoa.h>

@class DrawTestView;
@class DrawTestToolbarController;

@interface DrawTestDocument : NSDocument
{
    IBOutlet DrawTestView *drawView;
    IBOutlet NSDrawer *debugDrawer;

    DrawTestToolbarController *toolbarController;
}

// Debug menu
- (IBAction)dumpSVGToConsole:(id)sender;
- (IBAction)toggleDebugDrawer:(id)sender;
- (IBAction)runWindowResizeTest:(id)sender;

@end
