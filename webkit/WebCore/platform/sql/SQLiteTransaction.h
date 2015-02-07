

#ifndef SQLiteTransaction_h
#define SQLiteTransaction_h

#include <wtf/Noncopyable.h>

namespace WebCore {

class SQLiteDatabase;

class SQLiteTransaction : public Noncopyable
{
public:
    SQLiteTransaction(SQLiteDatabase& db);
    ~SQLiteTransaction();
    
    void begin();
    void commit();
    void rollback();
    void stop();
    
    bool inProgress() const { return m_inProgress; }
private:
    SQLiteDatabase& m_db;
    bool m_inProgress;

};

} // namespace WebCore

#endif // SQLiteTransation_H

