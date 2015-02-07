

// This header contains the WebFrame SPI.

#import <WebKit/WebFrame.h>
#import <JavaScriptCore/JSBase.h>

#if !defined(ENABLE_NETSCAPE_PLUGIN_API)
#define ENABLE_NETSCAPE_PLUGIN_API 1
#endif

@class DOMDocumentFragment;
@class DOMNode;
@class WebIconFetcher;
@class WebScriptObject;

// Keys for accessing the values in the page cache dictionary.
extern NSString *WebPageCacheEntryDateKey;
extern NSString *WebPageCacheDataSourceKey;
extern NSString *WebPageCacheDocumentViewKey;

extern NSString *WebFrameMainDocumentError;
extern NSString *WebFrameHasPlugins;
extern NSString *WebFrameHasUnloadListener;
extern NSString *WebFrameUsesDatabases;
extern NSString *WebFrameUsesGeolocation;
extern NSString *WebFrameUsesApplicationCache;
extern NSString *WebFrameCanSuspendActiveDOMObjects;

typedef enum {
    WebFrameLoadTypeStandard,
    WebFrameLoadTypeBack,
    WebFrameLoadTypeForward,
    WebFrameLoadTypeIndexedBackForward, // a multi-item hop in the backforward list
    WebFrameLoadTypeReload,
    WebFrameLoadTypeReloadAllowingStaleData,
    WebFrameLoadTypeSame,               // user loads same URL again (but not reload button)
    WebFrameLoadTypeInternal,           // maps to WebCore::FrameLoadTypeRedirectWithLockedBackForwardList
    WebFrameLoadTypeReplace,
    WebFrameLoadTypeReloadFromOrigin,
    WebFrameLoadTypeBackWMLDeckNotAccessible
} WebFrameLoadType;

@interface WebFrame (WebPrivate)
- (BOOL)_isDescendantOfFrame:(WebFrame *)frame;
- (void)_setShouldCreateRenderers:(BOOL)f;
- (NSColor *)_bodyBackgroundColor;
- (BOOL)_isFrameSet;
- (BOOL)_firstLayoutDone;
- (WebFrameLoadType)_loadType;

// These methods take and return NSRanges based on the root editable element as the positional base.
// This fits with AppKit's idea of an input context. These methods are slow compared to their DOMRange equivalents.
// You should use WebView's selectedDOMRange and setSelectedDOMRange whenever possible.
- (NSRange)_selectedNSRange;
- (void)_selectNSRange:(NSRange)range;

- (BOOL)_isDisplayingStandaloneImage;

- (unsigned) _pendingFrameUnloadEventCount;

- (WebIconFetcher *)fetchApplicationIcon:(id)target
                                selector:(SEL)selector;

- (void)_setIsDisconnected:(bool)isDisconnected;
- (void)_setExcludeFromTextSearch:(bool)exclude;

#if ENABLE_NETSCAPE_PLUGIN_API
- (void)_recursive_resumeNullEventsForAllNetscapePlugins;
- (void)_recursive_pauseNullEventsForAllNetscapePlugins;
#endif

// Pause a given CSS animation or transition on the target node at a specific time.
// If the animation or transition is already paused, it will update its pause time.
// This method is only intended to be used for testing the CSS animation and transition system.
- (BOOL)_pauseAnimation:(NSString*)name onNode:(DOMNode *)node atTime:(NSTimeInterval)time;
- (BOOL)_pauseTransitionOfProperty:(NSString*)name onNode:(DOMNode*)node atTime:(NSTimeInterval)time;

// Returns the total number of currently running animations (includes both CSS transitions and CSS animations).
- (unsigned) _numberOfActiveAnimations;

- (void)_replaceSelectionWithFragment:(DOMDocumentFragment *)fragment selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle;
- (void)_replaceSelectionWithText:(NSString *)text selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace;
- (void)_replaceSelectionWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace;

- (NSMutableDictionary *)_cacheabilityDictionary;
@end
