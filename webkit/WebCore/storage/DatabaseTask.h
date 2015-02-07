
#ifndef DatabaseTask_h
#define DatabaseTask_h

#if ENABLE(DATABASE)
#include "ExceptionCode.h"
#include "PlatformString.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

namespace WebCore {

class Database;
class DatabaseThread;
class SQLValue;
class SQLCallback;
class SQLTransaction;
class VersionChangeCallback;

class DatabaseTask : public ThreadSafeShared<DatabaseTask> {
    friend class Database;
public:
    virtual ~DatabaseTask();

    void performTask();

    Database* database() const { return m_database; }
    bool isComplete() const { return m_complete; }

protected:
    DatabaseTask(Database*);

private:
    virtual void doPerformTask() = 0;
#ifndef NDEBUG
    virtual const char* debugTaskName() const = 0;
#endif

    void lockForSynchronousScheduling();
    void waitForSynchronousCompletion();

    Database* m_database;

    bool m_complete;

    OwnPtr<Mutex> m_synchronousMutex;
    OwnPtr<ThreadCondition> m_synchronousCondition;
};

class DatabaseOpenTask : public DatabaseTask {
public:
    static PassRefPtr<DatabaseOpenTask> create(Database* db) { return adoptRef(new DatabaseOpenTask(db)); }

    ExceptionCode exceptionCode() const { return m_code; }
    bool openSuccessful() const { return m_success; }

private:
    DatabaseOpenTask(Database*);

    virtual void doPerformTask();
#ifndef NDEBUG
    virtual const char* debugTaskName() const;
#endif

    ExceptionCode m_code;
    bool m_success;
};

class DatabaseCloseTask : public DatabaseTask {
public:
    static PassRefPtr<DatabaseCloseTask> create(Database* db) { return adoptRef(new DatabaseCloseTask(db)); }

private:
    DatabaseCloseTask(Database*);

    virtual void doPerformTask();
#ifndef NDEBUG
    virtual const char* debugTaskName() const;
#endif
};

class DatabaseTransactionTask : public DatabaseTask {
public:
    static PassRefPtr<DatabaseTransactionTask> create(PassRefPtr<SQLTransaction> transaction) { return adoptRef(new DatabaseTransactionTask(transaction)); }

    SQLTransaction* transaction() const { return m_transaction.get(); }

    virtual ~DatabaseTransactionTask();
private:
    DatabaseTransactionTask(PassRefPtr<SQLTransaction>);

    virtual void doPerformTask();
#ifndef NDEBUG
    virtual const char* debugTaskName() const;
#endif

    RefPtr<SQLTransaction> m_transaction;
};

class DatabaseTableNamesTask : public DatabaseTask {
public:
    static PassRefPtr<DatabaseTableNamesTask> create(Database* db) { return adoptRef(new DatabaseTableNamesTask(db)); }

    Vector<String>& tableNames() { return m_tableNames; }

private:
    DatabaseTableNamesTask(Database*);

    virtual void doPerformTask();
#ifndef NDEBUG
    virtual const char* debugTaskName() const;
#endif

    Vector<String> m_tableNames;
};

} // namespace WebCore

#endif // ENABLE(DATABASE)
#endif // DatabaseTask_h
