

#ifndef RenderSkinCombo_h
#define RenderSkinCombo_h

#include "RenderSkinAndroid.h"
#include "SkRect.h"

class SkCanvas;

namespace WebCore {

// This is very similar to RenderSkinButton - maybe they should be the same class?
class RenderSkinCombo : public RenderSkinAndroid
{
public:
    RenderSkinCombo();
    virtual ~RenderSkinCombo() {}

    /**
     * Initialize the class before use. Uses the AssetManager to initialize any bitmaps the class may use.
     */
    static void Init(android::AssetManager*);

    /**
     * Draw the provided Node on the SkCanvas, using the dimensions provided by
     * x,y,w,h.  Return true if we did not draw, and WebKit needs to draw it,
     * false otherwise.
     */
    static bool Draw(SkCanvas* , Node* , int x, int y, int w, int h);

    // The image is wider than the RenderObject, so this accounts for that.
    static int extraWidth() { return arrowMargin; }
    
private:
    
    static const int arrowMargin = 22;
}; 

} // WebCore

#endif
