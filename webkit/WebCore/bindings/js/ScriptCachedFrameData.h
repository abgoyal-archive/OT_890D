

#ifndef ScriptCachedFrameData_h
#define ScriptCachedFrameData_h

#include <runtime/Protect.h>

namespace WebCore {
    class Frame;
    class JSDOMWindow;
    class DOMWindow;

    class ScriptCachedFrameData  {
    public:
        ScriptCachedFrameData(Frame*);
        ~ScriptCachedFrameData();

        void restore(Frame*);
        void clear();
        DOMWindow* domWindow() const;

    private:
        JSC::ProtectedPtr<JSDOMWindow> m_window;
    };

} // namespace WebCore

#endif // ScriptCachedFrameData_h
