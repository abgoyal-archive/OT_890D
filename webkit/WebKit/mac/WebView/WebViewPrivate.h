

#import <WebKit/WebView.h>
#import <WebKit/WebFramePrivate.h>

#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 1
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
#define WebNSInteger int
#define WebNSUInteger unsigned int
#else
#define WebNSInteger NSInteger
#define WebNSUInteger NSUInteger
#endif

@class NSError;
@class WebFrame;
@class WebInspector;
@class WebPreferences;
@class WebTextIterator;

@protocol WebFormDelegate;

extern NSString *_WebCanGoBackKey;
extern NSString *_WebCanGoForwardKey;
extern NSString *_WebEstimatedProgressKey;
extern NSString *_WebIsLoadingKey;
extern NSString *_WebMainFrameIconKey;
extern NSString *_WebMainFrameTitleKey;
extern NSString *_WebMainFrameURLKey;
extern NSString *_WebMainFrameDocumentKey;

// pending public WebElementDictionary keys
extern NSString *WebElementTitleKey;             // NSString of the title of the element (used by Safari)
extern NSString *WebElementSpellingToolTipKey;   // NSString of a tooltip representing misspelling or bad grammar (used internally)
extern NSString *WebElementIsContentEditableKey; // NSNumber indicating whether the inner non-shared node is content editable (used internally)

// other WebElementDictionary keys
extern NSString *WebElementLinkIsLiveKey;        // NSNumber of BOOL indictating whether the link is live or not

#if ENABLE_DASHBOARD_SUPPORT
typedef enum {
    WebDashboardBehaviorAlwaysSendMouseEventsToAllWindows,
    WebDashboardBehaviorAlwaysSendActiveNullEventsToPlugIns,
    WebDashboardBehaviorAlwaysAcceptsFirstMouse,
    WebDashboardBehaviorAllowWheelScrolling,
    WebDashboardBehaviorUseBackwardCompatibilityMode
} WebDashboardBehavior;
#endif

@interface WebController : NSTreeController {
    IBOutlet WebView *webView;
}
- (WebView *)webView;
- (void)setWebView:(WebView *)newWebView;
@end

@interface WebView (WebViewEditingActionsPendingPublic)

- (void)outdent:(id)sender;

@end

@interface WebView (WebPendingPublic)

- (void)scheduleInRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode;
- (void)unscheduleFromRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode;

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection;

- (void)setMainFrameDocumentReady:(BOOL)mainFrameDocumentReady;

- (void)setTabKeyCyclesThroughElements:(BOOL)cyclesElements;
- (BOOL)tabKeyCyclesThroughElements;

- (void)scrollDOMRangeToVisible:(DOMRange *)range;

// setHoverFeedbackSuspended: can be called by clients that want to temporarily prevent the webView
// from displaying feedback about mouse position. Each WebDocumentView class that displays feedback
// about mouse position should honor this setting.
- (void)setHoverFeedbackSuspended:(BOOL)newValue;
- (BOOL)isHoverFeedbackSuspended;

- (void)setScriptDebugDelegate:(id)delegate;

- (id)scriptDebugDelegate;

- (BOOL)shouldClose;

- (NSAppleEventDescriptor *)aeDescByEvaluatingJavaScriptFromString:(NSString *)script;

// Support for displaying multiple text matches.
// These methods might end up moving into a protocol, so different document types can specify
// whether or not they implement the protocol. For now we'll just deal with HTML.
// These methods are still in flux; don't rely on them yet.
- (BOOL)canMarkAllTextMatches;
- (WebNSUInteger)markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag highlight:(BOOL)highlight limit:(WebNSUInteger)limit;
- (void)unmarkAllTextMatches;
- (NSArray *)rectsForTextMatches;

// Support for disabling registration with the undo manager. This is equivalent to the methods with the same names on NSTextView.
- (BOOL)allowsUndo;
- (void)setAllowsUndo:(BOOL)flag;

- (void)setPageSizeMultiplier:(float)multiplier;

- (float)pageSizeMultiplier;

// Commands for doing page zoom.  Will end up in WebView (WebIBActions) <NSUserInterfaceValidations>
- (BOOL)canZoomPageIn;
- (IBAction)zoomPageIn:(id)sender;
- (BOOL)canZoomPageOut;
- (IBAction)zoomPageOut:(id)sender;
- (BOOL)canResetPageZoom;
- (IBAction)resetPageZoom:(id)sender;

// Sets a master volume control for all media elements in the WebView. Valid values are 0..1.
- (void)setMediaVolume:(float)volume;
- (float)mediaVolume;

@end

@interface WebView (WebPrivate)

- (WebInspector *)inspector;

- (void)setBackgroundColor:(NSColor *)backgroundColor;

- (NSColor *)backgroundColor;

- (void)_loadBackForwardListFromOtherView:(WebView *)otherView;


- (void)_dispatchPendingLoadRequests;

+ (NSArray *)_supportedFileExtensions;

+ (BOOL)canShowFile:(NSString *)path;

+ (NSString *)suggestedFileExtensionForMIMEType: (NSString *)MIMEType;

+ (NSString *)_standardUserAgentWithApplicationName:(NSString *)applicationName;

+ (BOOL)canCloseAllWebViews;

// May well become public
- (void)_setFormDelegate:(id<WebFormDelegate>)delegate;
- (id<WebFormDelegate>)_formDelegate;

- (BOOL)_isClosed;

// _close is now replaced by public method -close. It remains here only for backward compatibility
// until callers can be weaned off of it.
- (void)_close;

// Indicates if the WebView is in the midst of a user gesture.
- (BOOL)_isProcessingUserGesture;

// SPI for DumpRenderTree
- (void)_updateActiveState;

+ (void)_registerViewClass:(Class)viewClass representationClass:(Class)representationClass forURLScheme:(NSString *)URLScheme;

+ (void)_unregisterViewClassAndRepresentationClassForMIMEType:(NSString *)MIMEType;

+ (BOOL)_canHandleRequest:(NSURLRequest *)request;

+ (NSString *)_decodeData:(NSData *)data;

+ (void)_setAlwaysUsesComplexTextCodePath:(BOOL)f;
// This is the old name of the above method. Needed for Safari versions that call it.
+ (void)_setAlwaysUseATSU:(BOOL)f;

- (NSCachedURLResponse *)_cachedResponseForURL:(NSURL *)URL;

#if ENABLE_DASHBOARD_SUPPORT
- (void)_addScrollerDashboardRegions:(NSMutableDictionary *)regions;
- (NSDictionary *)_dashboardRegions;

- (void)_setDashboardBehavior:(WebDashboardBehavior)behavior to:(BOOL)flag;
- (BOOL)_dashboardBehavior:(WebDashboardBehavior)behavior;
#endif

+ (void)_setShouldUseFontSmoothing:(BOOL)f;
+ (BOOL)_shouldUseFontSmoothing;

- (void)_setCatchesDelegateExceptions:(BOOL)f;
- (BOOL)_catchesDelegateExceptions;

// These two methods are useful for a test harness that needs a consistent appearance for the focus rings
// regardless of OS X version.
+ (void)_setUsesTestModeFocusRingColor:(BOOL)f;
+ (BOOL)_usesTestModeFocusRingColor;

- (void)setAlwaysShowVerticalScroller:(BOOL)flag;

- (BOOL)alwaysShowVerticalScroller;

- (void)setAlwaysShowHorizontalScroller:(BOOL)flag;

- (BOOL)alwaysShowHorizontalScroller;

- (void)setProhibitsMainFrameScrolling:(BOOL)prohibits;

- (void)_setAdditionalWebPlugInPaths:(NSArray *)newPaths;

- (void)_setInViewSourceMode:(BOOL)flag;

- (BOOL)_inViewSourceMode;

- (void)_attachScriptDebuggerToAllFrames;

- (void)_detachScriptDebuggerFromAllFrames;

- (BOOL)defersCallbacks; // called by QuickTime plug-in
- (void)setDefersCallbacks:(BOOL)defer; // called by QuickTime plug-in

- (BOOL)usesPageCache;
- (void)setUsesPageCache:(BOOL)usesPageCache;

- (WebHistoryItem *)_globalHistoryItem;

- (WebTextIterator *)textIteratorForRect:(NSRect)rect;

#if ENABLE_DASHBOARD_SUPPORT
// <rdar://problem/5217124> Clients other than Dashboard, don't use this.
// As of this writing, Dashboard uses this on Tiger, but not on Leopard or newer.
- (void)handleAuthenticationForResource:(id)identifier challenge:(NSURLAuthenticationChallenge *)challenge fromDataSource:(WebDataSource *)dataSource;
#endif

- (void)_clearUndoRedoOperations;

/* Used to do fast (lower quality) scaling of images so that window resize can be quick. */
- (BOOL)_inFastImageScalingMode;
- (void)_setUseFastImageScalingMode:(BOOL)flag;

- (BOOL)_cookieEnabled;
- (void)_setCookieEnabled:(BOOL)enable;

// SPI for DumpRenderTree
- (void)_executeCoreCommandByName:(NSString *)name value:(NSString *)value;
- (void)_clearMainFrameName;

- (void)_setCustomHTMLTokenizerTimeDelay:(double)timeDelay;
- (void)_setCustomHTMLTokenizerChunkSize:(int)chunkSize;

- (id)_initWithFrame:(NSRect)f frameName:(NSString *)frameName groupName:(NSString *)groupName usesDocumentViews:(BOOL)usesDocumentViews;
- (BOOL)_usesDocumentViews;

- (void)setSelectTrailingWhitespaceEnabled:(BOOL)flag;
- (BOOL)isSelectTrailingWhitespaceEnabled;

- (void)setMemoryCacheDelegateCallsEnabled:(BOOL)suspend;
- (BOOL)areMemoryCacheDelegateCallsEnabled;

- (void)_setJavaScriptURLsAreAllowed:(BOOL)setJavaScriptURLsAreAllowed;

+ (NSCursor *)_pointingHandCursor;

// SPI for DumpRenderTree
- (BOOL)_isUsingAcceleratedCompositing;

// Which pasteboard text is coming from in editing delegate methods such as shouldInsertNode.
- (NSPasteboard *)_insertionPasteboard;

@end

@interface WebView (WebViewPrintingPrivate)
- (void)_adjustPrintingMarginsForHeaderAndFooter;

- (void)_drawHeaderAndFooter;
@end

@interface WebView (WebViewGrammarChecking)

// FIXME: These two methods should be merged into WebViewEditing when we're not in API freeze
- (BOOL)isGrammarCheckingEnabled;
#ifndef BUILDING_ON_TIGER
- (void)setGrammarCheckingEnabled:(BOOL)flag;

// FIXME: This method should be merged into WebIBActions when we're not in API freeze
- (void)toggleGrammarChecking:(id)sender;
#endif

@end

@interface WebView (WebViewTextChecking)

- (BOOL)isAutomaticQuoteSubstitutionEnabled;
- (BOOL)isAutomaticLinkDetectionEnabled;
- (BOOL)isAutomaticDashSubstitutionEnabled;
- (BOOL)isAutomaticTextReplacementEnabled;
- (BOOL)isAutomaticSpellingCorrectionEnabled;
#if !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD)
- (void)setAutomaticQuoteSubstitutionEnabled:(BOOL)flag;
- (void)toggleAutomaticQuoteSubstitution:(id)sender;
- (void)setAutomaticLinkDetectionEnabled:(BOOL)flag;
- (void)toggleAutomaticLinkDetection:(id)sender;
- (void)setAutomaticDashSubstitutionEnabled:(BOOL)flag;
- (void)toggleAutomaticDashSubstitution:(id)sender;
- (void)setAutomaticTextReplacementEnabled:(BOOL)flag;
- (void)toggleAutomaticTextReplacement:(id)sender;
- (void)setAutomaticSpellingCorrectionEnabled:(BOOL)flag;
- (void)toggleAutomaticSpellingCorrection:(id)sender;
#endif

@end

@interface WebView (WebViewEditingInMail)
- (void)_insertNewlineInQuotedContent;
- (void)_replaceSelectionWithNode:(DOMNode *)node matchStyle:(BOOL)matchStyle;
- (BOOL)_selectionIsCaret;
- (BOOL)_selectionIsAll;
@end

@interface NSObject (WebFrameLoadDelegatePrivate)
- (void)webView:(WebView *)sender didFirstLayoutInFrame:(WebFrame *)frame;

// didFinishDocumentLoadForFrame is sent when the document has finished loading, though not necessarily all
// of its subresources.
// FIXME 5259339: Currently this callback is not sent for (some?) pages loaded entirely from the cache.
- (void)webView:(WebView *)sender didFinishDocumentLoadForFrame:(WebFrame *)frame;

// Addresses 4192534.  SPI for now.
- (void)webView:(WebView *)sender didHandleOnloadEventsForFrame:(WebFrame *)frame;

- (void)webView:(WebView *)sender didFirstVisuallyNonEmptyLayoutInFrame:(WebFrame *)frame;

// For implementing the WebInspector's test harness
- (void)webView:(WebView *)webView didClearInspectorWindowObject:(WebScriptObject *)windowObject forFrame:(WebFrame *)frame;

@end

@interface NSObject (WebResourceLoadDelegatePrivate)
// Addresses <rdar://problem/5008925> - SPI for now
- (NSCachedURLResponse *)webView:(WebView *)sender resource:(id)identifier willCacheResponse:(NSCachedURLResponse *)response fromDataSource:(WebDataSource *)dataSource;
@end

#undef WebNSInteger
#undef WebNSUInteger
