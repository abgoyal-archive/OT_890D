

#ifndef ScriptCallStack_h
#define ScriptCallStack_h

#include "ScriptCallFrame.h"
#include "ScriptState.h"
#include "ScriptValue.h"
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>

namespace v8 {
    class Arguments;
}

namespace WebCore {

    class ScriptCallStack : public Noncopyable {
    public:
        ScriptCallStack(const v8::Arguments&, unsigned skipArgumentCount = 0);
        ~ScriptCallStack();

        const ScriptCallFrame& at(unsigned) const;
        // FIXME: implement retrieving and storing call stack trace
        unsigned size() const { return 1; }

        ScriptState* state() const { return m_scriptState.get(); }

    private:
        OwnPtr<ScriptState> m_scriptState;
        ScriptCallFrame m_lastCaller;
    };

} // namespace WebCore

#endif // ScriptCallStack_h
