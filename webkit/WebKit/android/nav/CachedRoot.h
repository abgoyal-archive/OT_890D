

#ifndef CachedRoot_H
#define CachedRoot_H

#include "CachedFrame.h"
#include "IntRect.h"
#include "SkPicture.h"
#include "wtf/Vector.h"

class SkRect;

namespace android {

class CachedHistory;
class CachedNode;

class CachedRoot : public CachedFrame {
public:
    enum ImeAction {
        FAILURE = -1,
        NEXT    = 0,
        GO      = 1,
        DONE    = 2
    };
    bool adjustForScroll(BestData* , Direction , WebCore::IntPoint* scrollPtr,
        bool findClosest);
    int checkForCenter(int x, int y) const;
    void checkForJiggle(int* ) const;
    bool checkRings(const WTF::Vector<WebCore::IntRect>& rings,
        const WebCore::IntRect& bounds) const;
    WebCore::IntPoint cursorLocation() const;
    // This method returns the desired ImeAction for the textfield where the
    // mouse cursor currently is.  If the mouse cursor is not on a textfield,
    // it will return FAILURE
    ImeAction cursorTextFieldAction() const;
    int documentHeight() { return mContents.height(); }
    int documentWidth() { return mContents.width(); }
    const CachedNode* findAt(const WebCore::IntRect& , const CachedFrame** ,
        int* x, int* y, bool checkForHidden) const;
    const WebCore::IntRect& focusBounds() const { return mFocusBounds; }
    WebCore::IntPoint focusLocation() const;
    SkPicture* getPicture() { return mPicture; }
    int getAndResetSelectionEnd();
    int getAndResetSelectionStart();
    int getBlockLeftEdge(int x, int y, float scale) const;
    void getSimulatedMousePosition(WebCore::IntPoint* ) const;
    void init(WebCore::Frame* , CachedHistory* );
    bool innerDown(const CachedNode* , BestData* ) const;
    bool innerLeft(const CachedNode* , BestData* ) const;
    void innerMove(const CachedNode* ,BestData* bestData, Direction , 
        WebCore::IntPoint* scroll, bool firstCall);
    bool innerRight(const CachedNode* , BestData* ) const;
    bool innerUp(const CachedNode* , BestData* ) const;
    WebCore::String imageURI(int x, int y) const;
    bool maskIfHidden(BestData* ) const;
    const CachedNode* moveCursor(Direction , const CachedFrame** , WebCore::IntPoint* scroll);
    void reset();
    CachedHistory* rootHistory() const { return mHistory; }
    bool scrollDelta(WebCore::IntRect& cursorRingBounds, Direction , int* delta);
    const WebCore::IntRect& scrolledBounds() const { return mScrolledBounds; }
    void setCursor(CachedFrame* , CachedNode* );
    void setCachedFocus(CachedFrame* , CachedNode* );
    void setFocusBounds(const WebCore::IntRect& r) { mFocusBounds = r; }
    void setTextGeneration(int textGeneration) { mTextGeneration = textGeneration; }
    void setMaxScroll(int x, int y) { mMaxXScroll = x; mMaxYScroll = y; }
    void setPicture(SkPicture* picture) { mPicture = picture; }
    void setScrollOnly(bool state) { mScrollOnly = state; }
    void setSelection(int start, int end) { mSelectionStart = start; mSelectionEnd = end; }
    void setupScrolledBounds() const { mScrolledBounds = mViewBounds; }
    void setVisibleRect(const WebCore::IntRect& r) { mViewBounds = r; }
    int textGeneration() const { return mTextGeneration; }
    int width() const { return mPicture ? mPicture->width() : 0; }
private:
    CachedHistory* mHistory;
    SkPicture* mPicture;
    WebCore::IntRect mFocusBounds; // dom text input focus node bounds
    mutable WebCore::IntRect mScrolledBounds; // view bounds + amount visible as result of scroll
    int mTextGeneration;
    int mMaxXScroll;
    int mMaxYScroll;
    // These two are ONLY used when the tree is rebuilt and the focus is a textfield/area
    int mSelectionStart;
    int mSelectionEnd;
    bool mScrollOnly;
#if DUMP_NAV_CACHE
public:
    class Debug {
public:
        CachedRoot* base() const;
        void print() const;
    } mDebug;
#endif
};

}

#endif
