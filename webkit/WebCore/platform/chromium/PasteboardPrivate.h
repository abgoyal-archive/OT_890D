

#ifndef PasteboardPrivate_h
#define PasteboardPrivate_h

namespace WebCore {

    class PasteboardPrivate
    {
    public:
        enum ClipboardFormat {
            HTMLFormat,
            BookmarkFormat,
            WebSmartPasteFormat,
        };
    };

} // namespace WebCore

#endif
