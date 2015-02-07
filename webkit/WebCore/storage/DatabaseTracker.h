

#ifndef DatabaseTracker_h
#define DatabaseTracker_h

#if ENABLE(DATABASE)

#include "DatabaseDetails.h"
#include "PlatformString.h"
#include "SQLiteDatabase.h"
#include "StringHash.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class Database;
class DatabaseTrackerClient;
class Document;
class OriginQuotaManager;
class SecurityOrigin;

struct SecurityOriginHash;
struct SecurityOriginTraits;

class DatabaseTracker {
public:
    void setDatabaseDirectoryPath(const String&);
    const String& databaseDirectoryPath() const;

    bool canEstablishDatabase(Document*, const String& name, const String& displayName, unsigned long estimatedSize);
    void setDatabaseDetails(SecurityOrigin*, const String& name, const String& displayName, unsigned long estimatedSize);
    String fullPathForDatabase(SecurityOrigin*, const String& name, bool createIfDoesNotExist = true);

    void origins(Vector<RefPtr<SecurityOrigin> >& result);
    bool databaseNamesForOrigin(SecurityOrigin*, Vector<String>& result);

    DatabaseDetails detailsForNameAndOrigin(const String&, SecurityOrigin*);

    void addOpenDatabase(Database*);
    void removeOpenDatabase(Database*);

    unsigned long long usageForDatabase(const String&, SecurityOrigin*);
    unsigned long long usageForOrigin(SecurityOrigin*);
    unsigned long long quotaForOrigin(SecurityOrigin*);
    void setQuota(SecurityOrigin*, unsigned long long);
    
    void deleteAllDatabases();
    void deleteOrigin(SecurityOrigin*);
    void deleteDatabase(SecurityOrigin*, const String& name);

    void setClient(DatabaseTrackerClient*);
    
    // From a secondary thread, must be thread safe with its data
    void scheduleNotifyDatabaseChanged(SecurityOrigin*, const String& name);
    
    OriginQuotaManager& originQuotaManager();
    
    static DatabaseTracker& tracker();

    bool hasEntryForOrigin(SecurityOrigin*);

private:
    DatabaseTracker();

    String trackerDatabasePath() const;
    void openTrackerDatabase(bool createIfDoesNotExist);

    String originPath(SecurityOrigin*) const;
    
    bool hasEntryForDatabase(SecurityOrigin*, const String& databaseIdentifier);
    
    bool addDatabase(SecurityOrigin*, const String& name, const String& path);
    void populateOrigins();
    
    bool deleteDatabaseFile(SecurityOrigin*, const String& name);

    SQLiteDatabase m_database;

    typedef HashMap<RefPtr<SecurityOrigin>, unsigned long long, SecurityOriginHash> QuotaMap;
    Mutex m_quotaMapGuard;
    mutable OwnPtr<QuotaMap> m_quotaMap;

    typedef HashSet<Database*> DatabaseSet;
    typedef HashMap<String, DatabaseSet*> DatabaseNameMap;
    typedef HashMap<RefPtr<SecurityOrigin>, DatabaseNameMap*, SecurityOriginHash> DatabaseOriginMap;

    Mutex m_openDatabaseMapGuard;
    mutable OwnPtr<DatabaseOriginMap> m_openDatabaseMap;

    OwnPtr<OriginQuotaManager> m_quotaManager;

    String m_databaseDirectoryPath;
    
    DatabaseTrackerClient* m_client;

    std::pair<SecurityOrigin*, DatabaseDetails>* m_proposedDatabase;

#ifndef NDEBUG
    ThreadIdentifier m_thread;
#endif

    static void scheduleForNotification();
    static void notifyDatabasesChanged(void*);
};

} // namespace WebCore

#endif // ENABLE(DATABASE)
#endif // DatabaseTracker_h
