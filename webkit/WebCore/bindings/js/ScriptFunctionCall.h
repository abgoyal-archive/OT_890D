

#ifndef ScriptFunctionCall_h
#define ScriptFunctionCall_h

#include "PlatformString.h"
#include "ScriptObject.h"
#include "ScriptState.h"

#include <runtime/ArgList.h>

namespace JSC {
    class UString;
    class JSValue;
}

namespace WebCore {
    class ScriptValue;
    class ScriptString;

    class ScriptFunctionCall {
    public:
        ScriptFunctionCall(ScriptState* exec, const ScriptObject& thisObject, const String& name);
        virtual ~ScriptFunctionCall() {};

        void appendArgument(const ScriptObject&);
        void appendArgument(const ScriptString&);
        void appendArgument(const ScriptValue&);
        void appendArgument(const String&);
        void appendArgument(const JSC::UString&);
        void appendArgument(JSC::JSValue);
        void appendArgument(long long);
        void appendArgument(unsigned int);
        void appendArgument(int);
        void appendArgument(bool);
        ScriptValue call(bool& hadException, bool reportExceptions = true);
        ScriptValue call();
        ScriptObject construct(bool& hadException, bool reportExceptions = true);

    protected:
        ScriptState* m_exec;
        ScriptObject m_thisObject;
        String m_name;
        JSC::MarkedArgumentBuffer m_arguments;
    };

} // namespace WebCore

#endif // ScriptFunctionCall
