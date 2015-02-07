

// This is a Private header (containing SPI), despite the fact that its name
// does not contain the word Private.

// FIXME: Does Safari really need to use this any more? AppKit added autohidesScrollers
// in Panther, and that was the original reason we needed this view in Safari.

// FIXME: <rdar://problem/5898985> Mail currently expects this header to define WebCoreScrollbarAlwaysOn.
extern const int WebCoreScrollbarAlwaysOn;

@interface WebDynamicScrollBarsView : NSScrollView {
    int hScroll; // FIXME: Should be WebCore::ScrollbarMode if this was an ObjC++ header.
    int vScroll; // Ditto.
    BOOL hScrollModeLocked;
    BOOL vScrollModeLocked;
    BOOL suppressLayout;
    BOOL suppressScrollers;
    BOOL inUpdateScrollers;
    unsigned inUpdateScrollersLayoutPass;
}
- (void)setAllowsHorizontalScrolling:(BOOL)flag; // This method is used by Safari, so it cannot be removed.
@end
