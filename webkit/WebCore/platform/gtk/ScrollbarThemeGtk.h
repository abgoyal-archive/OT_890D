

#ifndef ScrollbarThemeGtk_h
#define ScrollbarThemeGtk_h

#include "ScrollbarTheme.h"

namespace WebCore {

class ScrollbarThemeGtk : public ScrollbarTheme {
public:
    virtual ~ScrollbarThemeGtk();

    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar);
};

}
#endif
