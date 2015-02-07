

#ifndef V8IsolatedWorld_h
#define V8IsolatedWorld_h

#include <v8.h>

#include "V8DOMMap.h"
#include "V8Index.h"
#include "V8Utilities.h"
#include "ScriptSourceCode.h"  // for WebCore::ScriptSourceCode

namespace WebCore {

    class V8Proxy;

    // V8IsolatedWorld
    //
    // V8IsolatedWorld represents a isolated execution environment for
    // JavaScript.  Each isolated world executes in parallel with the main
    // JavaScript world.  An isolated world has access to the same DOM data
    // structures as the main world but none of the JavaScript pointers.
    //
    // It is an error to ever share a JavaScript pointer between two isolated
    // worlds or between an isolated world and the main world.  Because
    // isolated worlds have access to the DOM, they need their own DOM wrappers
    // to avoid having pointers to the main world's DOM wrappers (which are
    // JavaScript objects).
    //
    class V8IsolatedWorld {
    public:
        ~V8IsolatedWorld();

        // Evaluate JavaScript in a new isolated world.  The script has access
        // to the DOM of the document associated with |proxy|.
        static void evaluate(const Vector<ScriptSourceCode>& sources, V8Proxy* proxy, int extensionGroup);

        // Returns the isolated world associated with
        // v8::Context::GetEntered().  Because worlds are isolated, the entire
        // JavaScript call stack should be from the same isolated world.
        // Returns NULL if the entered context is from the main world.
        //
        // FIXME: Consider edge cases with DOM mutation events that might
        // violate this invariant.
        //
        static V8IsolatedWorld* getEntered();

        v8::Handle<v8::Context> context() { return m_context; }

        DOMDataStore* getDOMDataStore() const { return m_domDataStore.getStore(); }

    private:
        // The lifetime of an isolated world is managed by the V8 garbage
        // collector.  In particular, the object created by this constructor is
        // freed when |context| is garbage collected.
        explicit V8IsolatedWorld(v8::Handle<v8::Context> context);

        // The v8::Context for the isolated world.  This object is keep on the
        // heap as long as |m_context| has not been garbage collected.
        v8::Persistent<v8::Context> m_context;

        // The backing store for the isolated world's DOM wrappers.  This class
        // doesn't have visibility into the wrappers.  This handle simply helps
        // manage their lifetime.
        DOMDataStoreHandle m_domDataStore;
    };

} // namespace WebCore

#endif // V8IsolatedWorld_h
