

#ifndef ScriptState_h
#define ScriptState_h

#include <v8.h>

namespace WebCore {
    class Node;
    class Page;
    class Frame;

    class ScriptState {
    public:
        ScriptState() { }
        ScriptState(Frame* frame);

        bool hadException() { return !m_exception.IsEmpty(); }
        void setException(v8::Local<v8::Value> exception)
        {
            m_exception = exception;
        }
        v8::Local<v8::Value> exception() { return m_exception; }

        Frame* frame() const { return m_frame; }

    private:
        v8::Local<v8::Value> m_exception;
        Frame* m_frame;
    };

    ScriptState* scriptStateFromNode(Node*);
    ScriptState* scriptStateFromPage(Page*);
}

#endif // ScriptState_h
