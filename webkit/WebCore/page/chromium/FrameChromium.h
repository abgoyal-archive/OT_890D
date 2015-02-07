

#ifndef FrameChromium_h
#define FrameChromium_h

#include "Frame.h"

namespace WebCore {

    // printRect is only used for the width/height ratio. Their absolute values aren't used.
    void computePageRectsForFrame(Frame*, const IntRect& printRect, float headerHeight, float footerHeight, float userScaleFactor, Vector<IntRect>& pages, int& pageHeight);

}

#endif
