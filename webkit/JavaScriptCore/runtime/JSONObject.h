

#ifndef JSONObject_h
#define JSONObject_h

#include "JSObject.h"

namespace JSC {

    class Stringifier;

    class JSONObject : public JSObject {
    public:
        JSONObject(PassRefPtr<Structure> structure)
            : JSObject(structure)
        {
        }

        static PassRefPtr<Structure> createStructure(JSValue prototype)
        {
            return Structure::create(prototype, TypeInfo(ObjectType));
        }

        static void markStringifiers(MarkStack&, Stringifier*);

    private:
        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
    };

} // namespace JSC

#endif // JSONObject_h
