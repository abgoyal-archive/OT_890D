

#ifndef android_graphics_DEFINED
#define android_graphics_DEFINED

#include "wtf/Vector.h"

#include "SkTypes.h"

class SkCanvas;

namespace WebCore {
    class IntRect;
    class GraphicsContext;
}

SkCanvas* android_gc2canvas(WebCore::GraphicsContext* gc);

// Data and methods for cursor rings

// used to inflate node cache entry
#define CURSOR_RING_HIT_TEST_RADIUS 5

// used to inval rectangle enclosing pressed state of ring
#define CURSOR_RING_OUTER_DIAMETER SkFixedToScalar(SkIntToFixed(13)>>2) // 13/4 == 3.25

struct CursorRing {
public:
    enum Flavor {
        NORMAL_FLAVOR,
        FAKE_FLAVOR,
        NORMAL_ANIMATING,
        FAKE_ANIMATING,
        ANIMATING_COUNT = 2
    };

    static void DrawRing(SkCanvas* ,
        const Vector<WebCore::IntRect>& rects, Flavor );
};

#endif
