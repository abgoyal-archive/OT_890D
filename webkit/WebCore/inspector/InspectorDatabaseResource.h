

#ifndef InspectorDatabaseResource_h
#define InspectorDatabaseResource_h

#if ENABLE(DATABASE)

#include "Database.h"
#include "ScriptObject.h"
#include "ScriptState.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class InspectorFrontend;
    
    class InspectorDatabaseResource : public RefCounted<InspectorDatabaseResource> {
    public:
        static PassRefPtr<InspectorDatabaseResource> create(Database* database, const String& domain, const String& name, const String& version)
        {
            return adoptRef(new InspectorDatabaseResource(database, domain, name, version));
        }

        void bind(InspectorFrontend* frontend);
        void unbind();

    private:
        InspectorDatabaseResource(Database*, const String& domain, const String& name, const String& version);
        
        RefPtr<Database> m_database;
        String m_domain;
        String m_name;
        String m_version;
        bool m_scriptObjectCreated;

    };

} // namespace WebCore

#endif // ENABLE(DATABASE)

#endif // InspectorDatabaseResource_h
