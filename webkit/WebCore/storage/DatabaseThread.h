
#ifndef DatabaseThread_h
#define DatabaseThread_h

#if ENABLE(DATABASE)
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/MessageQueue.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

class Database;
class DatabaseTask;
class Document;

class DatabaseThread : public ThreadSafeShared<DatabaseThread> {
public:
    static PassRefPtr<DatabaseThread> create() { return adoptRef(new DatabaseThread); }
    ~DatabaseThread();

    bool start();
    void requestTermination();
    bool terminationRequested() const;

    void scheduleTask(PassRefPtr<DatabaseTask>);
    void scheduleImmediateTask(PassRefPtr<DatabaseTask>); // This just adds the task to the front of the queue - the caller needs to be extremely careful not to create deadlocks when waiting for completion.
    void unscheduleDatabaseTasks(Database*);

    void recordDatabaseOpen(Database*);
    void recordDatabaseClosed(Database*);
    ThreadIdentifier getThreadID() { return m_threadID; }

private:
    DatabaseThread();

    static void* databaseThreadStart(void*);
    void* databaseThread();

    Mutex m_threadCreationMutex;
    ThreadIdentifier m_threadID;
    RefPtr<DatabaseThread> m_selfRef;

    MessageQueue<RefPtr<DatabaseTask> > m_queue;

    // This set keeps track of the open databases that have been used on this thread.
    typedef HashSet<RefPtr<Database> > DatabaseSet;
    DatabaseSet m_openDatabaseSet;
};

} // namespace WebCore

#endif // ENABLE(DATABASE)
#endif // DatabaseThread_h
