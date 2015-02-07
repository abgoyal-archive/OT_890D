

#ifndef ScriptCachedPageData_h
#define ScriptCachedPageData_h

// We don't use WebKit's page caching, so this implementation is just a stub.

namespace WebCore {
    class Frame;
    class DOMWindow;

    class ScriptCachedFrameData  {
    public:
        ScriptCachedFrameData(Frame*) { }
        ~ScriptCachedFrameData() { }

        void restore(Frame*) { }
        void clear() { }
        DOMWindow* domWindow() const { return 0; }
    };

} // namespace WebCore

#endif // ScriptCachedPageData_h
