

#ifndef EllipsisBox_h
#define EllipsisBox_h

#include "InlineBox.h"

namespace WebCore {

class HitTestRequest;
class HitTestResult;

class EllipsisBox : public InlineBox {
public:
    EllipsisBox(RenderObject* obj, const AtomicString& ellipsisStr, InlineFlowBox* parent,
                int width, int height, int y, bool firstLine, InlineBox* markupBox)
        : InlineBox(obj, 0, y, width, firstLine, true, false, false, 0, 0, parent)
        , m_height(height)
        , m_str(ellipsisStr)
        , m_markupBox(markupBox)
    {
    }

    virtual void paint(RenderObject::PaintInfo&, int tx, int ty);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty);

private:
    virtual int height() const { return m_height; }

    int m_height;
    AtomicString m_str;
    InlineBox* m_markupBox;
};

} // namespace WebCore

#endif // EllipsisBox_h
