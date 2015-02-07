

#ifndef StringObjectThatMasqueradesAsUndefined_h
#define StringObjectThatMasqueradesAsUndefined_h

#include "JSGlobalObject.h"
#include "StringObject.h"
#include "UString.h"

namespace JSC {

    // WebCore uses this to make style.filter undetectable
    class StringObjectThatMasqueradesAsUndefined : public StringObject {
    public:
        static StringObjectThatMasqueradesAsUndefined* create(ExecState* exec, const UString& string)
        {
            return new (exec) StringObjectThatMasqueradesAsUndefined(exec,
                createStructure(exec->lexicalGlobalObject()->stringPrototype()), string);
        }

    private:
        StringObjectThatMasqueradesAsUndefined(ExecState* exec, PassRefPtr<Structure> structure, const UString& string)
            : StringObject(exec, structure, string)
        {
        }

        static PassRefPtr<Structure> createStructure(JSValue proto) 
        { 
            return Structure::create(proto, TypeInfo(ObjectType, MasqueradesAsUndefined)); 
        }

        virtual bool toBoolean(ExecState*) const { return false; }
    };
 
} // namespace JSC

#endif // StringObjectThatMasqueradesAsUndefined_h
