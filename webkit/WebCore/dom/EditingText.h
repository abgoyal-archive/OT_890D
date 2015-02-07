

#ifndef EditingText_h
#define EditingText_h

#include "Text.h"

namespace WebCore {

class EditingText : public Text {
public:
    EditingText(Document *impl, const String &text);
    EditingText(Document *impl);
    virtual ~EditingText();

    virtual bool rendererIsNeeded(RenderStyle *);
};

} // namespace WebCore

#endif // EditingText_h
