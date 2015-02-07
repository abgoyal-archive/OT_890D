

#ifndef SQLDatabase_h
#define SQLDatabase_h

#include "PlatformString.h"
#include <wtf/Threading.h>

#if COMPILER(MSVC)
#pragma warning(disable: 4800)
#endif

struct sqlite3;

namespace WebCore {

class DatabaseAuthorizer;
class SQLiteStatement;
class SQLiteTransaction;

extern const int SQLResultDone;
extern const int SQLResultError;
extern const int SQLResultOk;
extern const int SQLResultRow;
extern const int SQLResultSchema;
extern const int SQLResultFull;

class SQLiteDatabase : public Noncopyable {
    friend class SQLiteTransaction;
public:
    SQLiteDatabase();
    ~SQLiteDatabase();

    bool open(const String& filename);
    bool isOpen() const { return m_db; }
    void close();

    bool executeCommand(const String&);
    bool returnsAtLeastOneResult(const String&);
    
    bool tableExists(const String&);
    void clearAllTables();
    void runVacuumCommand();
    
    bool transactionInProgress() const { return m_transactionInProgress; }
    
    int64_t lastInsertRowID();
    int lastChanges();

    void setBusyTimeout(int ms);
    void setBusyHandler(int(*)(void*, int));
    
    void setFullsync(bool);
    
    // Gets/sets the maximum size in bytes
    // Depending on per-database attributes, the size will only be settable in units that are the page size of the database, which is established at creation
    // These chunks will never be anything other than 512, 1024, 2048, 4096, 8192, 16384, or 32768 bytes in size.
    // setMaximumSize() will round the size down to the next smallest chunk if the passed size doesn't align.
    int64_t maximumSize();
    void setMaximumSize(int64_t);
    
    // Gets the number of unused bytes in the database file.
    int64_t freeSpaceSize();

    // The SQLite SYNCHRONOUS pragma can be either FULL, NORMAL, or OFF
    // FULL - Any writing calls to the DB block until the data is actually on the disk surface
    // NORMAL - SQLite pauses at some critical moments when writing, but much less than FULL
    // OFF - Calls return immediately after the data has been passed to disk
    enum SynchronousPragma { SyncOff = 0, SyncNormal = 1, SyncFull = 2 };
    void setSynchronous(SynchronousPragma);
    
    int lastError();
    const char* lastErrorMsg();
    
    sqlite3* sqlite3Handle() const {
        ASSERT(currentThread() == m_openingThread);
        return m_db;
    }
    
    void setAuthorizer(PassRefPtr<DatabaseAuthorizer>);

    // (un)locks the database like a mutex
    void lock();
    void unlock();

private:
    static int authorizerFunction(void*, int, const char*, const char*, const char*, const char*);

    void enableAuthorizer(bool enable);
    
    int pageSize();
    
    sqlite3* m_db;
    int m_lastError;
    int m_pageSize;
    
    bool m_transactionInProgress;
    
    Mutex m_authorizerLock;
    RefPtr<DatabaseAuthorizer> m_authorizer;

    Mutex m_lockingMutex;
    ThreadIdentifier m_openingThread;
    
}; // class SQLiteDatabase

} // namespace WebCore

#endif
