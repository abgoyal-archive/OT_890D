

#ifndef PasteboardHelper_h
#define PasteboardHelper_h


#include "Frame.h"

#include <gtk/gtk.h>

namespace WebCore {

class PasteboardHelper {
public:
    virtual ~PasteboardHelper() {};

    virtual GtkClipboard* getCurrentTarget(Frame*) const = 0;
    virtual GtkClipboard* getClipboard(Frame*) const = 0;
    virtual GtkClipboard* getPrimary(Frame*) const = 0;
    virtual GtkTargetList* getCopyTargetList(Frame*) const = 0;
    virtual GtkTargetList* getPasteTargetList(Frame*) const = 0;
};

}

#endif // PasteboardHelper_h
