

#ifndef PrintContext_h
#define PrintContext_h

#include <wtf/Vector.h>

namespace WebCore {

class Frame;
class FloatRect;
class GraphicsContext;
class IntRect;

class PrintContext {
public:
    PrintContext(Frame*);
    ~PrintContext();

    int pageCount() const;

    void computePageRects(const FloatRect& printRect, float headerHeight, float footerHeight, float userScaleFactor, float& outPageHeight);

    // TODO: eliminate width param
    void begin(float width);

    // TODO: eliminate width param
    void spoolPage(GraphicsContext& ctx, int pageNumber, float width);

    void end();

protected:
    Frame* m_frame;
    Vector<IntRect> m_pageRects;
};

}

#endif
