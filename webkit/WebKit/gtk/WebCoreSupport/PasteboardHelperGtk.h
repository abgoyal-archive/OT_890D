

#ifndef PasteboardHelperGtk_h
#define PasteboardHelperGtk_h


#include "Frame.h"
#include "PasteboardHelper.h"

#include <gtk/gtk.h>

using namespace WebCore;

namespace WebKit {

class PasteboardHelperGtk : public PasteboardHelper {
public:
    PasteboardHelperGtk() { }
    virtual GtkClipboard* getCurrentTarget(Frame*) const;
    virtual GtkClipboard* getClipboard(Frame*) const;
    virtual GtkClipboard* getPrimary(Frame*) const;
    virtual GtkTargetList* getCopyTargetList(Frame*) const;
    virtual GtkTargetList* getPasteTargetList(Frame*) const;
};

}

#endif // PasteboardHelperGtk_h
