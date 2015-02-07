

// ScriptObjectQuarantine is used in JSC for wrapping DOM objects of the page
// before they are passed to Inspector's front-end. The wrapping prevents
// malicious scripts from gaining privileges. For V8, we are currently just
// passing the object itself, without any wrapping.

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

    bool getQuarantinedScriptObject(Database* database, ScriptObject& quarantinedObject);
    bool getQuarantinedScriptObject(Frame* frame, Storage* storage, ScriptObject& quarantinedObject);
    bool getQuarantinedScriptObject(Node* node, ScriptObject& quarantinedObject);
    bool getQuarantinedScriptObject(DOMWindow* domWindow, ScriptObject& quarantinedObject);

}

#endif // ScriptObjectQuarantine_h
