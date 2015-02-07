

#ifndef ScriptObjectQuarantine_h
#define ScriptObjectQuarantine_h

#include "ScriptState.h"

namespace WebCore {

    class Database;
    class DOMWindow;
    class Frame;
    class Node;
    class ScriptObject;
    class ScriptValue;
    class Storage;

    ScriptValue quarantineValue(ScriptState*, const ScriptValue&);

#if ENABLE(DATABASE)
    bool getQuarantinedScriptObject(Database* database, ScriptObject& quarantinedObject);
#endif
#if ENABLE(DOM_STORAGE)
    bool getQuarantinedScriptObject(Frame* frame, Storage* storage, ScriptObject& quarantinedObject);
#endif
    bool getQuarantinedScriptObject(Node* node, ScriptObject& quarantinedObject);
    bool getQuarantinedScriptObject(DOMWindow* domWindow, ScriptObject& quarantinedObject);

}

#endif // ScriptObjectQuarantine_h
