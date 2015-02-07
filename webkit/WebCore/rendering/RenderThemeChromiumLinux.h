

#ifndef RenderThemeChromiumLinux_h
#define RenderThemeChromiumLinux_h

#include "RenderThemeChromiumSkia.h"

namespace WebCore {

    class RenderThemeChromiumLinux : public RenderThemeChromiumSkia {
    public:
        static PassRefPtr<RenderTheme> create();
        virtual String extraDefaultStyleSheet();

        virtual Color systemColor(int cssValidId) const;

        // A method asking if the control changes its tint when the window has focus or not.
        virtual bool controlSupportsTints(const RenderObject*) const;

        // List Box selection color
        virtual Color activeListBoxSelectionBackgroundColor() const;
        virtual Color activeListBoxSelectionForegroundColor() const;
        virtual Color inactiveListBoxSelectionBackgroundColor() const;
        virtual Color inactiveListBoxSelectionForegroundColor() const;

    private:
        RenderThemeChromiumLinux();
        virtual ~RenderThemeChromiumLinux();

        // A general method asking if any control tinting is supported at all.
        virtual bool supportsControlTints() const;
    };

} // namespace WebCore

#endif // RenderThemeChromiumLinux_h
