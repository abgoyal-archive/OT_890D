

#ifndef FrameWin_H
#define FrameWin_H

#include "Frame.h"

// Forward declared so we don't need wingdi.h.
typedef struct HBITMAP__* HBITMAP;

namespace WebCore {

    HBITMAP imageFromSelection(Frame* frame, bool forceWhiteText);
    void computePageRectsForFrame(Frame*, const IntRect& printRect, float headerHeight, float footerHeight, float userScaleFactor, Vector<IntRect>& pages, int& pageHeight);

}

#endif
