

#import <Cocoa/Cocoa.h>

@class DrawTestView;
@class SVGTest;
@class TestViewerSplitView;

@interface TestController : NSObject {
    IBOutlet NSPanel *_testPanel;
    IBOutlet NSWindow *_testWindow;
    IBOutlet TestViewerSplitView *_splitView;

    IBOutlet NSArrayController *_testsArrayController;
    IBOutlet NSPopUpButton *_parentDirectoryPopup;
    IBOutlet NSTableView *_testsTableView;
    
    IBOutlet NSWindow *_compositeWindow;
    IBOutlet NSImageView *_compositeImageView;
    
@private
    NSString *_currentPath;
    NSArray *_tests;
    SVGTest *_selectedTest;
    
    DrawTestView *_drawView;
    NSImageView *_imageView;
}

+ (id)sharedController;

- (IBAction)showTestsPanel:(id)sender;
- (IBAction)showTestWindow:(id)sender;
- (IBAction)showCompositeWindow:(id)sender;

- (IBAction)browse:(id)sender;
- (IBAction)jumpToParentDirectory:(id)sender;
- (IBAction)openTestViewerForSelection:(id)sender;
- (IBAction)openSourceForSelection:(id)sender;
- (IBAction)openSelectionInViewer:(id)sender;
- (IBAction)toggleViewersScaleRule:(id)sender;

- (NSArray *)tests;
- (NSString *)currentPath;
- (void)setCurrentPath:(NSString *)newPath;
- (NSArray *)directoryHierarchy;

@end
