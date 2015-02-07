

#ifndef MessageQueue_h
#define MessageQueue_h

#include <limits>
#include <wtf/Assertions.h>
#include <wtf/Deque.h>
#include <wtf/Noncopyable.h>
#include <wtf/Threading.h>

namespace WTF {

    enum MessageQueueWaitResult {
        MessageQueueTerminated,       // Queue was destroyed while waiting for message.
        MessageQueueTimeout,          // Timeout was specified and it expired.
        MessageQueueMessageReceived,  // A message was successfully received and returned.
    };

    template<typename DataType>
    class MessageQueue : public Noncopyable {
    public:
        MessageQueue() : m_killed(false) { }
        
        void append(const DataType&);
        bool appendAndCheckEmpty(const DataType&);
        void prepend(const DataType&);
        bool waitForMessage(DataType&);
        template<typename Predicate>
        MessageQueueWaitResult waitForMessageFilteredWithTimeout(DataType&, Predicate&, double absoluteTime);
        void kill();

        bool tryGetMessage(DataType&);
        bool killed() const;

        // The result of isEmpty() is only valid if no other thread is manipulating the queue at the same time.
        bool isEmpty();

        static double infiniteTime() { return std::numeric_limits<double>::max(); }

    private:
        static bool alwaysTruePredicate(DataType&) { return true; }

        mutable Mutex m_mutex;
        ThreadCondition m_condition;
        Deque<DataType> m_queue;
        bool m_killed;
    };

    template<typename DataType>
    inline void MessageQueue<DataType>::append(const DataType& message)
    {
        MutexLocker lock(m_mutex);
        m_queue.append(message);
        m_condition.signal();
    }

    // Returns true if the queue was empty before the item was added.
    template<typename DataType>
    inline bool MessageQueue<DataType>::appendAndCheckEmpty(const DataType& message)
    {
        MutexLocker lock(m_mutex);
        bool wasEmpty = m_queue.isEmpty();
        m_queue.append(message);
        m_condition.signal();
        return wasEmpty;
    }

    template<typename DataType>
    inline void MessageQueue<DataType>::prepend(const DataType& message)
    {
        MutexLocker lock(m_mutex);
        m_queue.prepend(message);
        m_condition.signal();
    }

    template<typename DataType>
    inline bool MessageQueue<DataType>::waitForMessage(DataType& result)
    {
        MessageQueueWaitResult exitReason = waitForMessageFilteredWithTimeout(result, MessageQueue<DataType>::alwaysTruePredicate, infiniteTime());
        ASSERT(exitReason == MessageQueueTerminated || exitReason == MessageQueueMessageReceived);
        return exitReason == MessageQueueMessageReceived;
    }

    template<typename DataType>
    template<typename Predicate>
    inline MessageQueueWaitResult MessageQueue<DataType>::waitForMessageFilteredWithTimeout(DataType& result, Predicate& predicate, double absoluteTime)
    {
        MutexLocker lock(m_mutex);
        bool timedOut = false;

        DequeConstIterator<DataType> found = m_queue.end();
        while (!m_killed && !timedOut && (found = m_queue.findIf(predicate)) == m_queue.end())
            timedOut = !m_condition.timedWait(m_mutex, absoluteTime);

        ASSERT(!timedOut || absoluteTime != infiniteTime());

        if (m_killed)
            return MessageQueueTerminated;

        if (timedOut)
            return MessageQueueTimeout;

        ASSERT(found != m_queue.end());
        result = *found;
        m_queue.remove(found);
        return MessageQueueMessageReceived;
    }

    template<typename DataType>
    inline bool MessageQueue<DataType>::tryGetMessage(DataType& result)
    {
        MutexLocker lock(m_mutex);
        if (m_killed)
            return false;
        if (m_queue.isEmpty())
            return false;

        result = m_queue.first();
        m_queue.removeFirst();
        return true;
    }

    template<typename DataType>
    inline bool MessageQueue<DataType>::isEmpty()
    {
        MutexLocker lock(m_mutex);
        if (m_killed)
            return true;
        return m_queue.isEmpty();
    }

    template<typename DataType>
    inline void MessageQueue<DataType>::kill()
    {
        MutexLocker lock(m_mutex);
        m_killed = true;
        m_condition.broadcast();
    }

    template<typename DataType>
    inline bool MessageQueue<DataType>::killed() const
    {
        MutexLocker lock(m_mutex);
        return m_killed;
    }
} // namespace WTF

using WTF::MessageQueue;
// MessageQueueWaitResult enum and all its values.
using WTF::MessageQueueWaitResult;
using WTF::MessageQueueTerminated;
using WTF::MessageQueueTimeout;
using WTF::MessageQueueMessageReceived;

#endif // MessageQueue_h
