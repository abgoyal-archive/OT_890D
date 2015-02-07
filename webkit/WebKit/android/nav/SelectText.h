

#ifndef SELECT_TEXT_H
#define SELECT_TEXT_H

class SkPicture;
struct SkIRect;
struct SkIPoint;
class SkRegion;

class CopyPaste {
public:
    static void buildSelection(const SkPicture& , const SkIRect& area,
        const SkIRect& selStart, const SkIRect& selEnd, SkRegion* region);
    static SkIRect findClosest(const SkPicture& , const SkIRect& area,
        int x, int y);
};

#endif
