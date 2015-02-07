

#ifndef V8HiddenPropertyName_h
#define V8HiddenPropertyName_h

#include <v8.h>

namespace WebCore {

    class V8HiddenPropertyName {
    public:
        static v8::Handle<v8::String> objectPrototype();
        static v8::Handle<v8::String> isolatedWorld();

    private:
        static v8::Persistent<v8::String>* createString(const char* key);
    };

}

#endif // V8HiddenPropertyName_h
