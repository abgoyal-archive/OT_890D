

#ifndef Pasteboard_h
#define Pasteboard_h

#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(GTK)
#include <PasteboardHelper.h>
#endif

// FIXME: This class is too high-level to be in the platform directory, since it
// uses the DOM and makes calls to Editor. It should either be divested of its
// knowledge of the frame and editor or moved into the editing directory.

#if PLATFORM(MAC)
class NSFileWrapper;
class NSPasteboard;
class NSArray;
#endif

#if PLATFORM(WIN)
#include <windows.h>
typedef struct HWND__* HWND;
#endif

#if PLATFORM(CHROMIUM)
#include "PasteboardPrivate.h"
#endif

namespace WebCore {

#if PLATFORM(MAC)
extern NSString *WebArchivePboardType;
extern NSString *WebSmartPastePboardType;
extern NSString *WebURLNamePboardType;
extern NSString *WebURLPboardType;
extern NSString *WebURLsWithTitlesPboardType;
#endif

class CString;
class DocumentFragment;
class Frame;
class HitTestResult;
class KURL;
class Node;
class Range;
class String;
    
class Pasteboard : public Noncopyable {
public:
#if PLATFORM(MAC)
    //Helper functions to allow Clipboard to share code
    static void writeSelection(NSPasteboard* pasteboard, Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame);
    static void writeURL(NSPasteboard* pasteboard, NSArray* types, const KURL& url, const String& titleStr, Frame* frame);
#endif
    
    static Pasteboard* generalPasteboard();
    void writeSelection(Range*, bool canSmartCopyOrDelete, Frame*);
    void writeURL(const KURL&, const String&, Frame* = 0);
    void writeImage(Node*, const KURL&, const String& title);
#if PLATFORM(MAC)
    void writeFileWrapperAsRTFDAttachment(NSFileWrapper*);
#endif
    void clear();
    bool canSmartReplace();
    PassRefPtr<DocumentFragment> documentFragment(Frame*, PassRefPtr<Range>, bool allowPlainText, bool& chosePlainText);
    String plainText(Frame* = 0);
#if PLATFORM(QT)
    bool isSelectionMode() const;
    void setSelectionMode(bool selectionMode);
#endif

#if PLATFORM(GTK)
    void setHelper(PasteboardHelper*);
#endif

private:
    Pasteboard();
    ~Pasteboard();

#if PLATFORM(MAC)
    Pasteboard(NSPasteboard *);
    RetainPtr<NSPasteboard> m_pasteboard;
#endif

#if PLATFORM(WIN)
    HWND m_owner;
#endif

#if PLATFORM(GTK)
    PasteboardHelper* m_helper;
#endif

#if PLATFORM(QT)
    bool m_selectionMode;
#endif

#if PLATFORM(CHROMIUM)
    PasteboardPrivate p;
#endif
};

} // namespace WebCore

#endif // Pasteboard_h
